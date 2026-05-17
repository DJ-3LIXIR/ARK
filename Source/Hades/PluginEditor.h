/*
  ==============================================================================
    HADES - Plugin Editor Header
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// ============================================================
//  Colour Palette
// ============================================================
namespace HadesColours
{
    const juce::Colour black        { 0xFF0A0A0A };
    const juce::Colour darkChar     { 0xFF1A0A0A };
    const juce::Colour deepRed      { 0xFF3D0000 };
    const juce::Colour crimson      { 0xFF8B0000 };
    const juce::Colour brightCrim   { 0xFFCC1010 };
    const juce::Colour ember        { 0xFFFF4500 };
    const juce::Colour offWhite     { 0xFFE8DDD0 };
    const juce::Colour paleGold     { 0xFFCCAA55 };
    const juce::Colour sectionBorder{ 0xFF5C0A0A };
}

// ============================================================
//  FireEffect — blazing flame tongues rising from the bottom
//               of the amp grille, RMS-reactive
// ============================================================

// A single flame tongue: spawns at the bottom, rises, tapers, sways
struct FlameTongue
{
    float baseX   { 0.0f };  // x spawn position, normalised [0,1]
    float x       { 0.0f };  // current normalised x (drifts with sway)
    float height  { 0.0f };  // current pixel height of tongue
    float maxH    { 0.0f };  // peak pixel height
    float baseW   { 0.0f };  // half-width at base in pixels
    float life    { 0.0f };  // [0,1]  0=just born, 1=dead
    float speed   { 0.0f };  // life/tick increment (before level scaling)
    float sway    { 0.0f };  // phase offset for lateral sway
    float swayAmp { 0.0f };  // normalised sway amplitude
};

class FireEffect
{
public:
    // Dense pack of tongues — they overlap to form a continuous wall of fire
    static constexpr int kMaxTongues = 48;

    FireEffect()
    {
        rng.setSeedRandomly();
        for (auto& t : tongues)
            respawn (t, true, 0.0f);
    }

    // ----------------------------------------------------------------
    // Advance simulation one timer tick.
    // rawRms — processor.rmsLevel (post-output-volume RMS, ~0..0.5 typical)
    // ----------------------------------------------------------------
    void tick (float rawRms)
    {
        // Fast attack, slow release — boost the sensitivity multiplier so
        // even moderate playing reads as "loud"
        float target = juce::jlimit (0.0f, 1.0f, rawRms * 8.0f);
        if (target > smoothedLevel)
            smoothedLevel += (target - smoothedLevel) * 0.50f;  // snappier attack
        else
            smoothedLevel += (target - smoothedLevel) * 0.04f;  // long tail

        float lvl = smoothedLevel;

        // All tongues active even at low levels — fire is always present
        activeTongues = 20 + (int)std::round (lvl * (kMaxTongues - 20));

        // Phase moves much faster when loud — chaotic, urgent
        phase += 0.03f + lvl * 0.12f;

        // Speed scalar: loud = 6× faster burning/rising
        float speedMul = 1.0f + lvl * 5.0f;

        for (int i = 0; i < activeTongues; ++i)
        {
            auto& t = tongues[i];

            t.life += t.speed * speedMul;

            float swayOffset = std::sin (phase * 0.9f + t.sway)
                             + 0.4f * std::sin (phase * 2.1f + t.sway * 1.7f);
            t.x = t.baseX + swayOffset * t.swayAmp * (1.0f + lvl * 2.0f);

            float lifeCurve = std::sin (juce::MathConstants<float>::pi
                                        * std::pow (t.life, 0.55f));
            t.height = t.maxH * lifeCurve;

            if (t.life >= 1.0f)
                respawn (t, false, lvl);
        }
    }

    // ----------------------------------------------------------------
    // Draw flames.  Called before grid lines are drawn so fire glows
    // through the mesh.
    // ----------------------------------------------------------------
    void paint (juce::Graphics& g, juce::Rectangle<float> meshF) const
    {
        float lvl     = smoothedLevel;
        float mLeft   = meshF.getX();
        float mRight  = meshF.getRight();
        float mBottom = meshF.getBottom();
        float mTop    = meshF.getY();
        float mW      = meshF.getWidth();
        float mH      = meshF.getHeight();

        g.saveState();
        g.reduceClipRegion (meshF.toNearestInt().reduced (2));

        // ── 1. Permanent ember bed at the very bottom ─────────────────
        // Grows massively when loud — thick blazing base
        float bedH = 8.0f + lvl * 55.0f;
        juce::ColourGradient emberBed (
            juce::Colours::white.withAlpha (0.70f + lvl * 0.30f),
            mLeft + mW * 0.5f, mBottom,
            juce::Colour (0xFFFF4400).withAlpha (0.0f),
            mLeft + mW * 0.5f, mBottom - bedH,
            false
        );
        g.setGradientFill (emberBed);
        g.fillRect (mLeft, mBottom - bedH, mW, bedH);

        // ── 2. Each flame tongue ──────────────────────────────────────
        for (int i = 0; i < activeTongues; ++i)
        {
            const auto& t = tongues[i];
            if (t.height < 2.0f) continue;

            float cx      = mLeft + t.x * mW;
            float tipY    = mBottom - t.height;
            float baseHW  = t.baseW;
            float lifeAlpha = smoothLifeAlpha (t.life);

            float taperPow = 1.6f + lvl * 1.8f;

            // --- Outer crimson/red halo — very wide when loud -----------
            {
                float outerW = baseHW * (2.5f + lvl * 2.5f);
                float outerH = t.height * (1.10f + lvl * 0.40f);
                juce::ColourGradient outerGrad (
                    juce::Colour (0xFFAA0000).withAlpha ((0.35f + lvl * 0.45f) * lifeAlpha),
                    cx, mBottom,
                    juce::Colours::transparentBlack,
                    cx, tipY - outerH * 0.15f,
                    false
                );
                g.setGradientFill (outerGrad);
                juce::Path outerFlame;
                outerFlame.startNewSubPath (cx - outerW, mBottom);
                outerFlame.quadraticTo (cx - outerW * 0.3f,
                                        mBottom - outerH * 0.55f,
                                        cx, tipY - outerH * 0.15f);
                outerFlame.quadraticTo (cx + outerW * 0.3f,
                                        mBottom - outerH * 0.55f,
                                        cx + outerW, mBottom);
                outerFlame.closeSubPath();
                g.fillPath (outerFlame);
            }

            // --- Orange/ember mid flame --------------------------------
            {
                float midW = baseHW * (1.8f + lvl * 1.4f);
                float midH = t.height * (0.92f + lvl * 0.15f);
                juce::ColourGradient midGrad (
                    HadesColours::ember.withAlpha ((0.65f + lvl * 0.30f) * lifeAlpha),
                    cx, mBottom,
                    juce::Colour (0xFFCC2200).withAlpha (0.0f),
                    cx, tipY,
                    false
                );
                g.setGradientFill (midGrad);
                juce::Path midFlame;
                float midTipW = midW * std::pow (0.06f, 1.0f / taperPow);
                midFlame.startNewSubPath (cx - midW, mBottom);
                midFlame.cubicTo (cx - midW * 0.6f, mBottom - midH * 0.4f,
                                  cx - midTipW,      mBottom - midH * 0.75f,
                                  cx,                tipY);
                midFlame.cubicTo (cx + midTipW,      mBottom - midH * 0.75f,
                                  cx + midW * 0.6f,  mBottom - midH * 0.4f,
                                  cx + midW,         mBottom);
                midFlame.closeSubPath();
                g.fillPath (midFlame);
            }

            // --- Yellow-white inner core --------------------------------
            {
                float coreW = baseHW * (0.80f + lvl * 0.60f);
                float coreH = t.height * (0.65f + lvl * 0.25f);
                juce::ColourGradient coreGrad (
                    juce::Colours::white.withAlpha ((0.80f + lvl * 0.20f) * lifeAlpha),
                    cx, mBottom,
                    juce::Colour (0xFFFFAA00).withAlpha (0.0f),
                    cx, mBottom - coreH,
                    false
                );
                g.setGradientFill (coreGrad);
                juce::Path coreFlame;
                coreFlame.startNewSubPath (cx - coreW, mBottom);
                coreFlame.quadraticTo (cx, mBottom - coreH * 0.5f,
                                       cx, mBottom - coreH);
                coreFlame.quadraticTo (cx, mBottom - coreH * 0.5f,
                                       cx + coreW, mBottom);
                coreFlame.closeSubPath();
                g.fillPath (coreFlame);
            }
        }

        // ── 3. Background heat gradient ────────────────────────────────
        // At full level the wash fills the entire grille blood-red
        float washReach = 0.30f + lvl * 0.70f;
        juce::ColourGradient heatWash (
            juce::Colour (0xFF6A0000).withAlpha (0.70f + lvl * 0.28f),
            meshF.getCentreX(), mBottom,
            juce::Colours::transparentBlack,
            meshF.getCentreX(), mBottom - mH * washReach,
            false
        );
        g.setGradientFill (heatWash);
        g.fillRoundedRectangle (meshF.reduced (3.0f), 3.0f);

        // ── 4. Orange bloom — kicks in earlier, goes much brighter ─────
        if (lvl > 0.30f)
        {
            float t2 = (lvl - 0.30f) / 0.70f;
            // Two-layer bloom: deep orange base + blinding white-orange core
            juce::ColourGradient bloom (
                juce::Colour (0xFFFF5500).withAlpha (t2 * 0.45f),
                meshF.getCentreX(), mBottom,
                juce::Colours::transparentBlack,
                meshF.getCentreX(), mTop,
                false
            );
            g.setGradientFill (bloom);
            g.fillRoundedRectangle (meshF.reduced (3.0f), 3.0f);

            // White-hot core blast at the very bottom when peaking
            if (lvl > 0.65f)
            {
                float corePulse = (lvl - 0.65f) / 0.35f;
                juce::ColourGradient coreBlast (
                    juce::Colours::white.withAlpha (corePulse * 0.55f),
                    meshF.getCentreX(), mBottom,
                    juce::Colours::transparentBlack,
                    meshF.getCentreX(), mBottom - mH * 0.4f,
                    false
                );
                g.setGradientFill (coreBlast);
                g.fillRoundedRectangle (meshF.reduced (3.0f), 3.0f);
            }
        }

        g.restoreState();
    }

    float getSmoothedLevel() const { return smoothedLevel; }

private:
    std::array<FlameTongue, kMaxTongues> tongues;
    juce::Random rng;
    float phase         { 0.0f };
    float smoothedLevel { 0.0f };
    int   activeTongues { 14 };

    void respawn (FlameTongue& t, bool randomStart, float lvl)
    {
        static int spawnCounter = 0;
        float slot = (float)(spawnCounter % kMaxTongues) / (float)kMaxTongues;
        spawnCounter++;

        t.baseX   = slot + (rng.nextFloat() - 0.5f) * (1.0f / kMaxTongues);
        t.baseX   = juce::jlimit (0.02f, 0.98f, t.baseX);
        t.x       = t.baseX;

        // Height: at full level tongues can reach 5× the grille height
        float baseMaxH = 35.0f + rng.nextFloat() * 45.0f;
        t.maxH    = baseMaxH * (0.4f + lvl * 4.5f);

        // Width: wide base, overlapping wall at full level
        t.baseW   = 16.0f + rng.nextFloat() * 22.0f + lvl * 40.0f;

        t.life    = randomStart ? rng.nextFloat() : 0.0f;
        t.speed   = 0.007f + rng.nextFloat() * 0.013f;
        t.sway    = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        t.swayAmp = (0.010f + rng.nextFloat() * 0.020f) * (1.0f + lvl * 2.5f);
    }

    // Alpha envelope: very fast fade-in (fire appears instantly from base),
    // then gentle fade-out near top/death
    static float smoothLifeAlpha (float life)
    {
        float fadeIn  = std::min (life * 8.0f, 1.0f);
        float fadeOut = std::min ((1.0f - life) * 2.5f, 1.0f);
        return fadeIn * fadeOut;
    }
};

// ============================================================
//  Hades Knob LookAndFeel — replicates painted knob style
// ============================================================
class HadesKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& /*slider*/) override
    {
        float cx = x + width  * 0.5f;
        float cy = y + height * 0.5f;
        float knobR = std::min (width, height) * 0.5f - 3.0f;

        // Knob shadow
        g.setColour (juce::Colour (0x50000000));
        g.fillEllipse (cx - knobR + 2, cy - knobR + 2, knobR * 2, knobR * 2);

        // Outer ring — dark recessed look
        g.setColour (juce::Colour (0xFF1A0000));
        g.fillEllipse (cx - knobR - 3, cy - knobR - 3, (knobR + 3) * 2, (knobR + 3) * 2);

        // Knob body — crimson/deep red gradient
        juce::ColourGradient knobGrad (
            HadesColours::brightCrim,  cx - knobR * 0.4f, cy - knobR * 0.4f,
            HadesColours::deepRed,     cx + knobR * 0.2f, cy + knobR,
            false
        );
        g.setGradientFill (knobGrad);
        g.fillEllipse (cx - knobR, cy - knobR, knobR * 2, knobR * 2);

        // Knob edge ring
        g.setColour (HadesColours::crimson.withAlpha (0.8f));
        g.drawEllipse (cx - knobR, cy - knobR, knobR * 2, knobR * 2, 1.5f);

        // Specular highlight — top-left
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillEllipse (cx - knobR * 0.5f, cy - knobR * 0.6f, knobR * 0.7f, knobR * 0.4f);

        // Indicator line — rotates with value
        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        float lineStartR = knobR * 0.3f;
        float lineEndR   = knobR - 4.0f;
        float sinA = std::sin (angle);
        float cosA = std::cos (angle);
        g.setColour (HadesColours::offWhite);
        g.drawLine (cx + sinA * lineStartR,
                    cy - cosA * lineStartR,
                    cx + sinA * lineEndR,
                    cy - cosA * lineEndR,
                    2.5f);
    }

    // Hide the label drawn by default — we paint our own beneath the knob
    juce::Label* createSliderTextBox (juce::Slider&) override
    {
        auto* label = new juce::Label();
        label->setVisible (false);
        return label;
    }
};

static HadesKnobLookAndFeel hadesKnobLAF;

// ============================================================
//  Power Button LookAndFeel — Hades-themed toggle
// ============================================================
class HadesPowerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g,
                                juce::Button& button,
                                const juce::Colour&,
                                bool isHighlighted,
                                bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        bool isOn   = button.getToggleState();

        if (isOn)
        {
            juce::ColourGradient grad (
                HadesColours::ember,   bounds.getCentreX(), bounds.getY(),
                HadesColours::crimson, bounds.getCentreX(), bounds.getBottom(),
                false
            );
            g.setGradientFill (grad);
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (HadesColours::ember.withAlpha (0.7f));
            g.drawRoundedRectangle (bounds, 4.0f, 1.5f);
        }
        else if (isHighlighted || isDown)
        {
            g.setColour (HadesColours::deepRed.withAlpha (0.8f));
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (HadesColours::crimson.withAlpha (0.5f));
            g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xFF120505));
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (HadesColours::crimson.withAlpha (0.35f));
            g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool, bool) override
    {
        bool isOn = button.getToggleState();
        g.setColour (isOn ? HadesColours::offWhite : HadesColours::crimson.withAlpha (0.6f));
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 10.0f,
                                      juce::Font::bold));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds(),
                          juce::Justification::centred, 1);
    }
};

static HadesPowerLookAndFeel hadesPowerLAF;

// ============================================================
//  Base Page — all pages inherit from this
// ============================================================
class HadesPage : public juce::Component
{
public:
    HadesPage() {}
    ~HadesPage() override {}

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        juce::ColourGradient gradient (
            HadesColours::black,   0.0f, 0.0f,
            HadesColours::deepRed, 0.0f, bounds.getHeight(),
            false
        );
        gradient.addColour (0.6, HadesColours::darkChar);
        gradient.addColour (0.85, juce::Colour (0xFF5C0000));
        g.setGradientFill (gradient);
        g.fillAll();
    }

    void resized() override {}
};

// ============================================================
//  AmpPage — two-panel layout with real Slider knobs
// ============================================================
class AmpPage : public HadesPage,
                private juce::Timer
{
public:
    AmpPage (const juce::String& ampLabel,
             HadesAudioProcessor& proc,
             std::atomic<bool>& activeFlag,
             const std::array<juce::String, 6>& paramIDs,
             const juce::StringArray& knobLabels)
        : label (ampLabel), knobNames (knobLabels), processor (proc)
    {
        // Power button — initial state from processor (restored on reopen)
        powerBtn.setLookAndFeel (&hadesPowerLAF);
        powerBtn.setClickingTogglesState (true);
        powerBtn.setToggleState (activeFlag.load(), juce::dontSendNotification);
        powerBtn.setButtonText (activeFlag.load() ? "ON" : "OFF");
        powerBtn.onClick = [this, &activeFlag]
        {
            bool on = powerBtn.getToggleState();
            activeFlag.store (on);
            powerBtn.setButtonText (on ? "ON" : "OFF");
            repaint();
        };
        addAndMakeVisible (powerBtn);

        // Create sliders and attachments
        for (int i = 0; i < 6; ++i)
        {
            knobs[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            knobs[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setLookAndFeel (&hadesKnobLAF);
            addAndMakeVisible (knobs[i]);

            attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                             (proc.apvts, paramIDs[i], knobs[i]);
        }

        startTimerHz (30);
    }

    ~AmpPage() override
    {
        stopTimer();
        powerBtn.setLookAndFeel (nullptr);
        for (auto& k : knobs)
            k.setLookAndFeel (nullptr);
    }

    void resized() override
    {
        auto ampRect = getLocalBounds().reduced (20);
        int splitY   = (int)(ampRect.getY() + ampRect.getHeight() * 0.6f);

        // Power button
        int btnX = ampRect.getX() + 86;
        int btnY = splitY + 14;
        powerBtn.setBounds (btnX, btnY, 46, 22);

        // Position knobs to match the painted layout
        float knobDiameter = 64.0f;   // knobR * 2
        float knobR        = 32.0f;
        float ampF_X       = (float)ampRect.getX();
        float ampF_Right   = (float)ampRect.getRight();
        float knobStartX   = ampF_X + 150.0f;
        float knobEndX     = ampF_Right - 30.0f;
        float knobSpacing  = (knobEndX - knobStartX) / 6.0f;
        float knobCY       = (float)splitY + (float)(ampRect.getBottom() - splitY) * 0.5f + 4.0f;

        for (int i = 0; i < 6; ++i)
        {
            float cx = knobStartX + knobSpacing * i + knobSpacing * 0.5f;
            knobs[i].setBounds ((int)(cx - knobR - 3),
                                (int)(knobCY - knobR - 3),
                                (int)((knobR + 3) * 2),
                                (int)((knobR + 3) * 2));
        }
    }

    void paint (juce::Graphics& g) override
    {
        HadesPage::paint (g);

        auto pageBounds = getLocalBounds();
        int margin = 20;
        auto ampRect = pageBounds.reduced (margin);
        auto ampF    = ampRect.toFloat();

        // Drop shadow
        g.setColour (juce::Colour (0x60000000));
        g.fillRoundedRectangle (ampF.translated (4.0f, 4.0f), 8.0f);

        // Amp body
        juce::ColourGradient bodyGrad (
            juce::Colour (0xFF222222), ampF.getX(), ampF.getY(),
            juce::Colour (0xFF0E0E0E), ampF.getX(), ampF.getBottom(), false
        );
        g.setGradientFill (bodyGrad);
        g.fillRoundedRectangle (ampF, 8.0f);

        g.setColour (juce::Colour (0xFF444444));
        g.drawLine (ampF.getX() + 8, ampF.getY() + 1, ampF.getRight() - 8, ampF.getY() + 1, 1.5f);
        g.drawLine (ampF.getX() + 1, ampF.getY() + 8, ampF.getX() + 1, ampF.getBottom() - 8, 1.0f);
        g.setColour (juce::Colour (0xFF080808));
        g.drawLine (ampF.getX() + 8, ampF.getBottom() - 1, ampF.getRight() - 8, ampF.getBottom() - 1, 2.0f);
        g.drawLine (ampF.getRight() - 1, ampF.getY() + 8, ampF.getRight() - 1, ampF.getBottom() - 8, 1.5f);
        g.setColour (juce::Colour (0xFF333333));
        g.drawRoundedRectangle (ampF, 8.0f, 2.0f);

        int splitY     = (int)(ampF.getY() + ampF.getHeight() * 0.6f);
        auto grilleArea = juce::Rectangle<float> (ampF.getX(), ampF.getY(),
                                                   ampF.getWidth(), (float)splitY - ampF.getY());
        auto panelArea  = juce::Rectangle<float> (ampF.getX(), (float)splitY,
                                                   ampF.getWidth(), ampF.getBottom() - (float)splitY);

        paintGrille       (g, grilleArea);
        paintControlPanel (g, panelArea);
    }

    juce::TextButton powerBtn;

protected:
    juce::String    label;
    juce::StringArray knobNames;
    juce::Slider    knobs[6];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[6];

private:
    HadesAudioProcessor& processor;
    FireEffect fireEffect;

    void timerCallback() override
    {
        fireEffect.tick (processor.rmsLevel.load());
        repaint();
    }

    void paintGrille (juce::Graphics& g, juce::Rectangle<float> area)
    {
        auto meshF = area.reduced (6.0f);

        g.setColour (juce::Colour (0xFF050505));
        g.fillRoundedRectangle (meshF, 4.0f);

        // ── Fire effect behind the mesh ──────────────────────
        fireEffect.paint (g, meshF);
        // ─────────────────────────────────────────────────────

        int gridSpacing = 7;
        g.saveState();
        g.reduceClipRegion (meshF.toNearestInt().reduced (2));
        for (float x = meshF.getX(); x < meshF.getRight(); x += gridSpacing)
        {
            g.setColour (juce::Colour (0xFF3A3A3A).withAlpha (0.6f));
            g.drawLine (x, meshF.getY(), x, meshF.getBottom(), 0.7f);
        }
        for (float y = meshF.getY(); y < meshF.getBottom(); y += gridSpacing)
        {
            g.setColour (juce::Colour (0xFF3A3A3A).withAlpha (0.5f));
            g.drawLine (meshF.getX(), y, meshF.getRight(), y, 0.7f);
        }
        g.restoreState();

        g.setColour (juce::Colour (0xFF666666));
        g.drawRoundedRectangle (meshF, 4.0f, 2.5f);
        g.setColour (juce::Colour (0xFF888888).withAlpha (0.3f));
        g.drawRoundedRectangle (meshF.reduced (1.5f), 3.0f, 1.0f);

        float plateW = 220.0f, plateH = 42.0f;
        float plateX = meshF.getCentreX() - plateW * 0.5f;
        float plateY = meshF.getCentreY() - plateH * 0.5f;
        auto plateF  = juce::Rectangle<float> (plateX, plateY, plateW, plateH);

        juce::ColourGradient plateGrad (
            juce::Colour (0xFF3A3A3A), plateF.getX(), plateF.getY(),
            juce::Colour (0xFF1A1A1A), plateF.getX(), plateF.getBottom(), false
        );
        g.setGradientFill (plateGrad);
        g.fillRoundedRectangle (plateF, 4.0f);
        g.setColour (HadesColours::ember.withAlpha (0.6f));
        g.drawRoundedRectangle (plateF, 4.0f, 1.5f);
        g.setColour (HadesColours::ember);
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 20.0f, juce::Font::bold));
        g.drawFittedText ("H A D E S", plateF.toNearestInt(), juce::Justification::centred, 1);
    }

    void paintControlPanel (juce::Graphics& g, juce::Rectangle<float> area)
    {
        juce::ColourGradient panelGrad (
            juce::Colour (0xFF1E1E1E), area.getX(), area.getY(),
            juce::Colour (0xFF141414), area.getX(), area.getBottom(), false
        );
        g.setGradientFill (panelGrad);
        g.fillRect (area);

        g.setColour (juce::Colour (0xFF555555));
        g.drawLine (area.getX() + 10, area.getY(), area.getRight() - 10, area.getY(), 2.0f);
        g.setColour (juce::Colour (0xFF222222));
        g.drawLine (area.getX() + 10, area.getY() + 2, area.getRight() - 10, area.getY() + 2, 1.0f);

        // Brand label
        g.setColour (HadesColours::offWhite.withAlpha (0.8f));
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::bold));
        g.drawFittedText ("HADES",
                          (int)area.getX() + 16, (int)area.getY() + 12, 80, 16,
                          juce::Justification::centredLeft, 1);
        g.setColour (HadesColours::ember.withAlpha (0.7f));
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::bold));
        g.drawFittedText (label,
                          (int)area.getX() + 16, (int)area.getY() + 30, 80, 14,
                          juce::Justification::centredLeft, 1);

        // Power LED
        bool isOn  = powerBtn.getToggleState();
        float ledX = area.getX() + 146.0f;
        float ledY = area.getY() + 25.0f;
        if (isOn)
        {
            g.setColour (HadesColours::ember.withAlpha (0.3f));
            g.fillEllipse (ledX - 8, ledY - 8, 16, 16);
            g.setColour (HadesColours::ember);
            g.fillEllipse (ledX - 5, ledY - 5, 10, 10);
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.fillEllipse (ledX - 3, ledY - 4, 5, 4);
        }
        else
        {
            g.setColour (juce::Colour (0xFF2A1000));
            g.fillEllipse (ledX - 5, ledY - 5, 10, 10);
            g.setColour (juce::Colour (0xFF1A0800));
            g.drawEllipse (ledX - 5, ledY - 5, 10, 10, 1.0f);
        }

        // Knob labels — painted beneath each slider
        float knobR       = 32.0f;
        float knobStartX  = area.getX() + 150.0f;
        float knobEndX    = area.getRight() - 30.0f;
        float knobSpacing = (knobEndX - knobStartX) / 6.0f;
        float knobCY      = area.getCentreY() + 4.0f;

        g.setColour (HadesColours::offWhite.withAlpha (0.7f));
        g.setFont (juce::FontOptions (9.5f));
        for (int i = 0; i < 6; ++i)
        {
            float cx = knobStartX + knobSpacing * i + knobSpacing * 0.5f;
            g.drawFittedText (knobNames[i],
                              (int)(cx - 30), (int)(knobCY + knobR + 6),
                              60, 14,
                              juce::Justification::centred, 1);
        }
    }
};

// ============================================================
//  Guitar Page
// ============================================================
class GuitarPage : public AmpPage
{
public:
    GuitarPage (HadesAudioProcessor& proc)
        : AmpPage ("GUITAR", proc, proc.guitarActive,
                   { "guitar_gain", "guitar_bass", "guitar_mids",
                     "guitar_treble", "guitar_presence", "guitar_master" },
                   { "GAIN", "BASS", "MIDS", "TREBLE", "PRESENCE", "MASTER" })
    {}
};

// ============================================================
//  Bass Page
// ============================================================
class BassPage : public AmpPage
{
public:
    BassPage (HadesAudioProcessor& proc)
        : AmpPage ("BASS", proc, proc.bassActive,
                   { "bass_gain", "bass_low", "bass_lomid",
                     "bass_himid", "bass_high", "bass_master" },
                   { "GAIN", "LOW", "LO-MID", "HI-MID", "HIGH", "MASTER" })
    {}
};

// ============================================================
//  Cabinet Page — same amp body DNA, 4 knobs
// ============================================================
class CabinetPage : public HadesPage,
                    private juce::Timer
{
public:
    CabinetPage (HadesAudioProcessor& proc)
        : processor (proc)
    {
        powerBtn.setLookAndFeel (&hadesPowerLAF);
        powerBtn.setClickingTogglesState (true);
        powerBtn.setToggleState (proc.cabActive.load(), juce::dontSendNotification);
        powerBtn.setButtonText (proc.cabActive.load() ? "ON" : "OFF");
        powerBtn.onClick = [this, &proc]
        {
            bool on = powerBtn.getToggleState();
            proc.cabActive.store (on);
            powerBtn.setButtonText (on ? "ON" : "OFF");
            repaint();
        };
        addAndMakeVisible (powerBtn);

        const juce::String paramIDs[4] = { "cab_mic", "cab_air", "cab_lowcut", "cab_mix" };
        for (int i = 0; i < 4; ++i)
        {
            knobs[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            knobs[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            knobs[i].setLookAndFeel (&hadesKnobLAF);
            addAndMakeVisible (knobs[i]);

            attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                             (proc.apvts, paramIDs[i], knobs[i]);
        }

        startTimerHz (30);
    }

    ~CabinetPage() override
    {
        stopTimer();
        powerBtn.setLookAndFeel (nullptr);
        for (auto& k : knobs)
            k.setLookAndFeel (nullptr);
    }

    void resized() override
    {
        auto ampRect = getLocalBounds().reduced (20);
        int splitY   = (int)(ampRect.getY() + ampRect.getHeight() * 0.6f);

        // Power button
        powerBtn.setBounds (ampRect.getX() + 86, splitY + 14, 46, 22);

        // 4 knobs in cabinet bottom panel
        float ampF_X      = (float)ampRect.getX();
        float ampF_Right  = (float)ampRect.getRight();
        float knobR       = 32.0f;
        float knobStartX  = ampF_X + 200.0f;
        float knobEndX    = ampF_Right - 80.0f;
        float knobSpacing = (knobEndX - knobStartX) / 4.0f;
        float bottomH     = (float)(ampRect.getBottom() - splitY);
        float knobCY      = (float)splitY + bottomH * 0.5f + 4.0f;

        for (int i = 0; i < 4; ++i)
        {
            float cx = knobStartX + knobSpacing * i + knobSpacing * 0.5f;
            knobs[i].setBounds ((int)(cx - knobR - 3),
                                (int)(knobCY - knobR - 3),
                                (int)((knobR + 3) * 2),
                                (int)((knobR + 3) * 2));
        }
    }

    void paint (juce::Graphics& g) override
    {
        HadesPage::paint (g);

        auto pageBounds = getLocalBounds();
        int margin = 20;
        auto ampRect = pageBounds.reduced (margin);
        auto ampF    = ampRect.toFloat();

        g.setColour (juce::Colour (0x60000000));
        g.fillRoundedRectangle (ampF.translated (4.0f, 4.0f), 8.0f);

        juce::ColourGradient bodyGrad (
            juce::Colour (0xFF222222), ampF.getX(), ampF.getY(),
            juce::Colour (0xFF0E0E0E), ampF.getX(), ampF.getBottom(), false
        );
        g.setGradientFill (bodyGrad);
        g.fillRoundedRectangle (ampF, 8.0f);

        g.setColour (juce::Colour (0xFF444444));
        g.drawLine (ampF.getX() + 8, ampF.getY() + 1, ampF.getRight() - 8, ampF.getY() + 1, 1.5f);
        g.drawLine (ampF.getX() + 1, ampF.getY() + 8, ampF.getX() + 1, ampF.getBottom() - 8, 1.0f);
        g.setColour (juce::Colour (0xFF080808));
        g.drawLine (ampF.getX() + 8, ampF.getBottom() - 1, ampF.getRight() - 8, ampF.getBottom() - 1, 2.0f);
        g.drawLine (ampF.getRight() - 1, ampF.getY() + 8, ampF.getRight() - 1, ampF.getBottom() - 8, 1.5f);
        g.setColour (juce::Colour (0xFF333333));
        g.drawRoundedRectangle (ampF, 8.0f, 2.0f);

        int splitY      = (int)(ampF.getY() + ampF.getHeight() * 0.6f);
        auto topArea    = juce::Rectangle<float> (ampF.getX(), ampF.getY(),
                                                   ampF.getWidth(), (float)splitY - ampF.getY());
        auto bottomArea = juce::Rectangle<float> (ampF.getX(), (float)splitY,
                                                   ampF.getWidth(), ampF.getBottom() - (float)splitY);

        paintCabinetTop    (g, topArea);
        paintCabinetBottom (g, bottomArea);
    }

    juce::TextButton powerBtn;

private:
    HadesAudioProcessor& processor;
    FireEffect      cabFireEffect;
    juce::Slider    knobs[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachments[4];

    const juce::StringArray knobLabels { "MIC", "AIR", "LOW CUT", "MIX" };

    void timerCallback() override
    {
        cabFireEffect.tick (processor.rmsLevel.load());
        repaint();
    }

    void paintCabinetTop (juce::Graphics& g, juce::Rectangle<float> area)
    {
        auto meshF = area.reduced (6.0f);
        g.setColour (juce::Colour (0xFF050505));
        g.fillRoundedRectangle (meshF, 4.0f);

        // ── Fire effect behind the mesh ──────────────────────
        cabFireEffect.paint (g, meshF);
        // ─────────────────────────────────────────────────────

        int gridSpacing = 7;
        g.saveState();
        g.reduceClipRegion (meshF.toNearestInt().reduced (2));
        for (float x = meshF.getX(); x < meshF.getRight(); x += gridSpacing)
        {
            g.setColour (juce::Colour (0xFF3A3A3A).withAlpha (0.6f));
            g.drawLine (x, meshF.getY(), x, meshF.getBottom(), 0.7f);
        }
        for (float y = meshF.getY(); y < meshF.getBottom(); y += gridSpacing)
        {
            g.setColour (juce::Colour (0xFF3A3A3A).withAlpha (0.5f));
            g.drawLine (meshF.getX(), y, meshF.getRight(), y, 0.7f);
        }
        g.restoreState();

        g.setColour (juce::Colour (0xFF666666));
        g.drawRoundedRectangle (meshF, 4.0f, 2.5f);
        g.setColour (juce::Colour (0xFF888888).withAlpha (0.3f));
        g.drawRoundedRectangle (meshF.reduced (1.5f), 3.0f, 1.0f);

        float plateW = 220.0f, plateH = 42.0f;
        float plateX = meshF.getCentreX() - plateW * 0.5f;
        float plateY = meshF.getCentreY() - plateH * 0.5f;
        auto plateF  = juce::Rectangle<float> (plateX, plateY, plateW, plateH);

        juce::ColourGradient plateGrad (
            juce::Colour (0xFF3A3A3A), plateF.getX(), plateF.getY(),
            juce::Colour (0xFF1A1A1A), plateF.getX(), plateF.getBottom(), false
        );
        g.setGradientFill (plateGrad);
        g.fillRoundedRectangle (plateF, 4.0f);
        g.setColour (HadesColours::ember.withAlpha (0.6f));
        g.drawRoundedRectangle (plateF, 4.0f, 1.5f);
        g.setColour (HadesColours::ember);
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 20.0f, juce::Font::bold));
        g.drawFittedText ("H A D E S", plateF.toNearestInt(), juce::Justification::centred, 1);
    }

    void paintCabinetBottom (juce::Graphics& g, juce::Rectangle<float> area)
    {
        juce::ColourGradient panelGrad (
            juce::Colour (0xFF1E1E1E), area.getX(), area.getY(),
            juce::Colour (0xFF141414), area.getX(), area.getBottom(), false
        );
        g.setGradientFill (panelGrad);
        g.fillRect (area);

        g.setColour (juce::Colour (0xFF555555));
        g.drawLine (area.getX() + 10, area.getY(), area.getRight() - 10, area.getY(), 2.0f);
        g.setColour (juce::Colour (0xFF222222));
        g.drawLine (area.getX() + 10, area.getY() + 2, area.getRight() - 10, area.getY() + 2, 1.0f);

        // Brand label
        g.setColour (HadesColours::offWhite.withAlpha (0.8f));
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::bold));
        g.drawFittedText ("HADES",
                          (int)area.getX() + 16, (int)area.getY() + 12, 80, 16,
                          juce::Justification::centredLeft, 1);
        g.setColour (HadesColours::ember.withAlpha (0.7f));
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::bold));
        g.drawFittedText ("CABINET",
                          (int)area.getX() + 16, (int)area.getY() + 30, 80, 14,
                          juce::Justification::centredLeft, 1);

        // Power LED
        bool isOn  = powerBtn.getToggleState();
        float ledX = area.getX() + 146.0f;
        float ledY = area.getY() + 25.0f;
        if (isOn)
        {
            g.setColour (HadesColours::ember.withAlpha (0.3f));
            g.fillEllipse (ledX - 8, ledY - 8, 16, 16);
            g.setColour (HadesColours::ember);
            g.fillEllipse (ledX - 5, ledY - 5, 10, 10);
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.fillEllipse (ledX - 3, ledY - 4, 5, 4);
        }
        else
        {
            g.setColour (juce::Colour (0xFF2A1000));
            g.fillEllipse (ledX - 5, ledY - 5, 10, 10);
            g.setColour (juce::Colour (0xFF1A0800));
            g.drawEllipse (ledX - 5, ledY - 5, 10, 10, 1.0f);
        }

        // Knob labels beneath sliders
        float knobR       = 32.0f;
        float knobStartX  = area.getX() + 200.0f;
        float knobEndX    = area.getRight() - 80.0f;
        float knobSpacing = (knobEndX - knobStartX) / 4.0f;
        float knobCY      = area.getCentreY() + 4.0f;

        g.setColour (HadesColours::offWhite.withAlpha (0.7f));
        g.setFont (juce::FontOptions (9.5f));
        for (int i = 0; i < 4; ++i)
        {
            float cx = knobStartX + knobSpacing * i + knobSpacing * 0.5f;
            g.drawFittedText (knobLabels[i],
                              (int)(cx - 30), (int)(knobCY + knobR + 6),
                              60, 14,
                              juce::Justification::centred, 1);
        }
    }
};

// ============================================================
//  Settings Page
// ============================================================
class HadesSettingsPage : public HadesPage,
                     private juce::Timer
{
public:
    HadesSettingsPage (HadesAudioProcessor& proc)
        : processor (proc)
    {
        startTimerHz (4); // refresh info labels a few times per second
    }

    ~HadesSettingsPage() override
    {
        stopTimer();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);
        int panelGap = 12;

        // Three equal-width panels side by side
        int panelW = (area.getWidth() - panelGap * 2) / 3;

        infoPanelBounds    = juce::Rectangle<int> (area.getX(),                          area.getY(), panelW, area.getHeight());
        audioPanelBounds   = juce::Rectangle<int> (area.getX() + panelW + panelGap,      area.getY(), panelW, area.getHeight());
        uiPanelBounds      = juce::Rectangle<int> (area.getX() + (panelW + panelGap) * 2, area.getY(), panelW, area.getHeight());
    }

    void paint (juce::Graphics& g) override
    {
        HadesPage::paint (g);

        paintPanel (g, infoPanelBounds,  "INFO");
        paintPanel (g, audioPanelBounds, "AUDIO");
        paintPanel (g, uiPanelBounds,    "DISPLAY");

        // ── Info panel contents ──
        paintInfoPanel (g);

        // ── Audio panel contents ──
        paintAudioPanel (g);

        // ── Display panel contents ──
        paintDisplayPanel (g);
    }

private:
    HadesAudioProcessor& processor;

    juce::Rectangle<int> infoPanelBounds;
    juce::Rectangle<int> audioPanelBounds;
    juce::Rectangle<int> uiPanelBounds;

    void timerCallback() override { repaint(); }

    // ── Draw a panel background with a title ──
    void paintPanel (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
    {
        auto bf = bounds.toFloat();

        // Panel background
        juce::ColourGradient panelGrad (
            juce::Colour (0xFF1C1010), bf.getX(), bf.getY(),
            juce::Colour (0xFF0E0808), bf.getX(), bf.getBottom(), false
        );
        g.setGradientFill (panelGrad);
        g.fillRoundedRectangle (bf, 6.0f);

        // Border
        g.setColour (HadesColours::sectionBorder.withAlpha (0.6f));
        g.drawRoundedRectangle (bf, 6.0f, 1.5f);

        // Title bar
        auto titleBar = bf.removeFromTop (32.0f);
        g.setColour (juce::Colour (0xFF2A0A0A));
        g.fillRoundedRectangle (titleBar.getX(), titleBar.getY(),
                                titleBar.getWidth(), titleBar.getHeight() + 6.0f, 6.0f);
        // Clip bottom corners of title bar
        g.setColour (juce::Colour (0xFF1C1010));
        g.fillRect (titleBar.getX(), titleBar.getBottom() - 6.0f,
                    titleBar.getWidth(), 6.0f);

        // Divider line
        g.setColour (HadesColours::crimson.withAlpha (0.5f));
        g.drawLine (bf.getX() + 8, titleBar.getBottom(),
                    bf.getRight() - 8, titleBar.getBottom(), 1.0f);

        // Title text
        g.setColour (HadesColours::ember);
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
        g.drawFittedText (title, titleBar.toNearestInt().reduced (10, 0),
                          juce::Justification::centredLeft, 1);
    }

    // ── Helper to draw a label : value row ──
    void drawRow (juce::Graphics& g, juce::Rectangle<int> panel, int row,
                  const juce::String& label, const juce::String& value)
    {
        int y = panel.getY() + 44 + row * 26;
        int lx = panel.getX() + 14;
        int vx = panel.getX() + panel.getWidth() / 2;
        int w  = panel.getWidth() / 2 - 14;

        g.setColour (HadesColours::offWhite.withAlpha (0.5f));
        g.setFont (juce::FontOptions (11.0f));
        g.drawFittedText (label, lx, y, w, 18, juce::Justification::centredLeft, 1);

        g.setColour (HadesColours::offWhite.withAlpha (0.85f));
        g.setFont (juce::FontOptions (11.0f));
        g.drawFittedText (value, vx, y, w, 18, juce::Justification::centredLeft, 1);
    }

    // ── Info panel ──
    void paintInfoPanel (juce::Graphics& g)
    {
        auto& b = infoPanelBounds;
        drawRow (g, b, 0, "PLUGIN",    "Hades");
        drawRow (g, b, 1, "VERSION",   "1.0.0");
        drawRow (g, b, 2, "FORMAT",    juce::String (processor.getWrapperTypeDescription (processor.wrapperType)));
        drawRow (g, b, 3, "LATENCY",   juce::String (processor.getLatencySamples()) + " samples");
        drawRow (g, b, 4, "SAMPLE RATE", juce::String ((int)processor.getSampleRate()) + " Hz");
        drawRow (g, b, 5, "BLOCK SIZE", juce::String (processor.getBlockSize()) + " samples");
        drawRow (g, b, 6, "CHANNELS",  juce::String (processor.getTotalNumInputChannels()) + " in / "
                                        + juce::String (processor.getTotalNumOutputChannels()) + " out");
        drawRow (g, b, 7, "AUTHOR",    "Jared Frazier");
    }

    // ── Audio panel ──
    void paintAudioPanel (juce::Graphics& g)
    {
        auto& b = audioPanelBounds;

        // Active amp
        int activeAmp = (int)*processor.apvts.getRawParameterValue ("active_amp");
        drawRow (g, b, 0, "ACTIVE AMP",   activeAmp == 0 ? "Guitar" : "Bass");

        // Power states
        drawRow (g, b, 1, "GUITAR PWR",   processor.guitarActive.load() ? "ON" : "OFF");
        drawRow (g, b, 2, "BASS PWR",     processor.bassActive.load()   ? "ON" : "OFF");
        drawRow (g, b, 3, "CABINET PWR",  processor.cabActive.load()    ? "ON" : "OFF");

        // RMS level
        float rms = processor.rmsLevel.load();
        float rmsDb = rms > 0.0f ? 20.0f * std::log10 (rms) : -100.0f;
        drawRow (g, b, 4, "RMS LEVEL", juce::String (rmsDb, 1) + " dB");

        // Output volume
        float outVol = *processor.apvts.getRawParameterValue ("output_volume");
        drawRow (g, b, 5, "OUTPUT VOL", juce::String ((int)(outVol * 100.0f)) + "%");

        // RMS meter bar
        int meterY = b.getY() + 44 + 6 * 26 + 6;
        int meterX = b.getX() + 14;
        int meterW = b.getWidth() - 28;
        int meterH = 10;

        g.setColour (juce::Colour (0xFF0A0A0A));
        g.fillRoundedRectangle ((float)meterX, (float)meterY, (float)meterW, (float)meterH, 3.0f);
        g.setColour (HadesColours::sectionBorder.withAlpha (0.4f));
        g.drawRoundedRectangle ((float)meterX, (float)meterY, (float)meterW, (float)meterH, 3.0f, 1.0f);

        // Fill based on RMS (map -60dB..0dB to 0..1)
        float normLevel = juce::jlimit (0.0f, 1.0f, (rmsDb + 60.0f) / 60.0f);
        float fillW = normLevel * (float)(meterW - 2);
        if (fillW > 0.0f)
        {
            juce::Colour meterCol = normLevel > 0.85f ? juce::Colour (0xFFFF2200)
                                  : normLevel > 0.6f  ? HadesColours::ember
                                  :                      HadesColours::crimson;
            g.setColour (meterCol);
            g.fillRoundedRectangle ((float)meterX + 1, (float)meterY + 1, fillW, (float)meterH - 2, 2.0f);
        }
    }

    // ── Display panel ──
    void paintDisplayPanel (juce::Graphics& g)
    {
        auto& b = uiPanelBounds;

        // Current page
        const juce::String pageNames[] = { "Guitar", "Bass", "Cabinet", "Settings" };
        int pageIdx = juce::jlimit (0, 3, processor.savedPageIndex);
        drawRow (g, b, 0, "CURRENT PAGE", pageNames[pageIdx]);

        // Preset info per section
        const juce::String guitarPresets[] = { "HADES", "Clean", "Crunch", "High Gain", "Metal", "Lead" };
        const juce::String bassPresets[]   = { "HADES", "Warm", "Punchy", "Gritty", "Distorted", "Clean DI" };
        const juce::String cabPresets[]    = { "4x12 Vintage", "4x12 Modern", "2x12 Open", "1x12 Combo", "2x12 Closed", "8x10 Bass" };

        int gi = juce::jlimit (0, 5, processor.savedPresetIndex[0]);
        int bi = juce::jlimit (0, 5, processor.savedPresetIndex[1]);
        int ci = juce::jlimit (0, 5, processor.savedPresetIndex[2]);

        drawRow (g, b, 1, "GUITAR PRESET", guitarPresets[gi]);
        drawRow (g, b, 2, "BASS PRESET",   bassPresets[bi]);
        drawRow (g, b, 3, "CAB PRESET",    cabPresets[ci]);

        // Cab type
        const juce::String cabTypes[] = { "4x12 Vintage", "4x12 Modern", "2x12 Open Back", "1x12 Combo", "2x12 Closed", "8x10 Bass" };
        int cabType = (int)*processor.apvts.getRawParameterValue ("cab_type");
        drawRow (g, b, 4, "CAB TYPE", cabTypes[juce::jlimit (0, 5, cabType)]);

        // Cab bypass
        bool cabBypass = *processor.apvts.getRawParameterValue ("cab_bypass") > 0.5f;
        drawRow (g, b, 5, "CAB BYPASS", cabBypass ? "ON" : "OFF");
    }
};

// ============================================================
//  Custom LookAndFeel for Hades nav buttons
// ============================================================
class HadesNavLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g,
                                juce::Button& button,
                                const juce::Colour&,
                                bool isHighlighted,
                                bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f, 4.0f);
        bool isOn   = button.getToggleState();

        if (isOn)
        {
            juce::ColourGradient grad (
                HadesColours::ember,   bounds.getCentreX(), bounds.getY(),
                HadesColours::crimson, bounds.getCentreX(), bounds.getBottom(), false
            );
            g.setGradientFill (grad);
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (HadesColours::ember.withAlpha (0.8f));
            g.drawRoundedRectangle (bounds, 4.0f, 1.5f);
        }
        else if (isHighlighted || isDown)
        {
            g.setColour (HadesColours::crimson.withAlpha (0.4f));
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (HadesColours::brightCrim.withAlpha (0.6f));
            g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xFF1A0505));
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (HadesColours::crimson.withAlpha (0.5f));
            g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool, bool) override
    {
        bool isOn = button.getToggleState();
        g.setColour (isOn ? HadesColours::offWhite : HadesColours::brightCrim);
        g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 11.5f, juce::Font::bold));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds(),
                          juce::Justification::centred, 1);
    }
};

static HadesNavLookAndFeel hadesLAF;

// ============================================================
//  Main Editor
// ============================================================
class HadesAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    HadesAudioProcessorEditor (HadesAudioProcessor&);
    ~HadesAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showPage (int index);
    void updatePresetBox (int pageIndex);

    HadesAudioProcessor& audioProcessor;

    // Nav buttons
    juce::TextButton btnGuitar   { "GUITAR" };
    juce::TextButton btnBass     { "BASS" };
    juce::TextButton btnCabinet  { "CABINET" };
    juce::TextButton btnSettings { "SETTINGS" };

    // Preset dropdown
    juce::ComboBox presetBox;

    // Pages — constructed with apvts reference
    GuitarPage   guitarPage;
    BassPage     bassPage;
    CabinetPage  cabinetPage;
    HadesSettingsPage settingsPage;

    int currentPage { 0 };
    int selectedPresetPerPage[3] { 0, 0, 0 };  // 0-based, mirrors processor.savedPresetIndex

    static constexpr int navHeight { 44 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HadesAudioProcessorEditor)
};
