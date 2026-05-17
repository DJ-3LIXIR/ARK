#pragma once

#include <JuceHeader.h>
#include "DSP/RoomTypePresets.h"
#include "DSP/OutputEQ.h"
#include "DSP/FrequencyResponseCalculator.h"

class APOLLOAudioProcessor;

class ReverbPage : public juce::Component,
                   public juce::AudioProcessorValueTreeState::Listener,
                   public juce::Timer
{
public:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    // --- Master power button (draws standard power icon) ---
    class PowerButton : public juce::TextButton
    {
    public:
        juce::Colour onColour  { juce::Colour::fromRGB (230, 176, 46) };
        juce::Colour offColour { juce::Colours::white.withAlpha (0.25f) };

        PowerButton() { setClickingTogglesState (true); }

        void paintButton (juce::Graphics& g, bool isHighlighted, bool /*isDown*/) override
        {
            auto b = getLocalBounds().toFloat().reduced (2.0f);
            auto c = b.getCentre();
            float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.34f;

            bool on = getToggleState();
            auto col = on ? onColour : offColour;
            if (isHighlighted) col = col.brighter (0.2f);

            g.setColour (col);

            juce::Path arc;
            float gap = 0.65f;
            arc.addCentredArc (c.x, c.y, r, r, 0.0f,
                               -juce::MathConstants<float>::halfPi + gap,
                               juce::MathConstants<float>::pi * 1.5f - gap,
                               true);
            g.strokePath (arc, juce::PathStrokeType (1.6f));
            g.drawLine (c.x, c.y, c.x, c.y - r * 1.15f, 1.6f);
        }
    };

    // --- EQ Band button (draws filter-type shape icon) ---
    class EQBandButton : public juce::TextButton
    {
    public:
        enum FilterType { HighPass, LowShelf, Bell, HighShelf, LowPass };

        FilterType filterType = Bell;
        juce::Colour onColour  { juce::Colour::fromRGB (230, 176, 46) };
        juce::Colour offColour { juce::Colours::white.withAlpha (0.25f) };

        EQBandButton() { setClickingTogglesState (true); }

        void paintButton (juce::Graphics& g, bool isHighlighted, bool /*isDown*/) override
        {
            auto b = getLocalBounds().toFloat().reduced (3.0f);
            bool on = getToggleState();
            auto col = on ? onColour : offColour;
            if (isHighlighted) col = col.brighter (0.2f);

            g.setColour (col);

            juce::Path shape;
            float l = b.getX(), r = b.getRight();
            float t = b.getY(), bt = b.getBottom();
            float cx = b.getCentreX(), cy = b.getCentreY();
            float w = b.getWidth(), h = b.getHeight();

            switch (filterType)
            {
                case HighPass:
                    // Slope up from bottom-left, then flat to the right
                    shape.startNewSubPath (l, bt);
                    shape.lineTo (cx, cy);
                    shape.lineTo (r, cy);
                    break;

                case LowShelf:
                    // Flat high on left, slope down, flat low on right
                    shape.startNewSubPath (l, t + h * 0.25f);
                    shape.lineTo (cx - w * 0.2f, t + h * 0.25f);
                    shape.lineTo (cx + w * 0.2f, bt - h * 0.25f);
                    shape.lineTo (r, bt - h * 0.25f);
                    break;

                case Bell:
                    // Diamond / peak shape
                    shape.startNewSubPath (l, cy);
                    shape.lineTo (cx, t + h * 0.1f);
                    shape.lineTo (r, cy);
                    shape.lineTo (cx, bt - h * 0.1f);
                    shape.closeSubPath();
                    break;

                case HighShelf:
                    // Flat low on left, slope up, flat high on right
                    shape.startNewSubPath (l, bt - h * 0.25f);
                    shape.lineTo (cx - w * 0.2f, bt - h * 0.25f);
                    shape.lineTo (cx + w * 0.2f, t + h * 0.25f);
                    shape.lineTo (r, t + h * 0.25f);
                    break;

                case LowPass:
                    // Flat on the left, slope down to bottom-right
                    shape.startNewSubPath (l, cy);
                    shape.lineTo (cx, cy);
                    shape.lineTo (r, bt);
                    break;
            }

            g.strokePath (shape, juce::PathStrokeType (1.8f));
        }
    };

    // --- Waveform shape button (Sine / Square / Noise icon) ---
    class WaveformButton : public juce::TextButton
    {
    public:
        enum WaveformType { Sine, Square, Noise };
        WaveformType waveType = Sine;
        juce::Colour onColour  { juce::Colour::fromRGB (230, 176, 46) };
        juce::Colour offColour { juce::Colours::white.withAlpha (0.25f) };

        WaveformButton() { setClickingTogglesState (true); }

        void paintButton (juce::Graphics& g, bool isHighlighted, bool /*isDown*/) override
        {
            auto b = getLocalBounds().toFloat().reduced (3.0f);
            bool on = getToggleState();
            auto col = on ? onColour : offColour;
            if (isHighlighted) col = col.brighter (0.2f);

            g.setColour (col);

            juce::Path shape;
            float l = b.getX(), r = b.getRight();
            float cy = b.getCentreY();
            float amp = b.getHeight() * 0.32f;

            switch (waveType)
            {
                case Sine:
                {
                    shape.startNewSubPath (l, cy);
                    float qw = (r - l) / 4.0f;
                    shape.cubicTo (l + qw, cy - amp * 2.2f,
                                   l + qw * 2, cy - amp * 2.2f,
                                   l + qw * 2, cy);
                    shape.cubicTo (l + qw * 3, cy + amp * 2.2f,
                                   r, cy + amp * 2.2f,
                                   r, cy);
                    break;
                }
                case Square:
                {
                    float mx = (l + r) * 0.5f;
                    shape.startNewSubPath (l, cy);
                    shape.lineTo (l, cy - amp);
                    shape.lineTo (mx, cy - amp);
                    shape.lineTo (mx, cy + amp);
                    shape.lineTo (r, cy + amp);
                    shape.lineTo (r, cy);
                    break;
                }
                case Noise:
                {
                    int steps = 8;
                    float stepW = (r - l) / (float) steps;
                    const float offsets[] = { 0.6f, -0.9f, 0.4f, -0.55f, 0.85f, -0.7f, 0.35f, -0.45f };
                    shape.startNewSubPath (l, cy);
                    for (int i = 0; i < steps; ++i)
                        shape.lineTo (l + (i + 1) * stepW, cy + offsets[i] * amp * 1.6f);
                    break;
                }
            }

            g.strokePath (shape, juce::PathStrokeType (1.6f));
        }
    };

    // --- Time / Note mode toggle button ---
    class TimeNoteButton : public juce::TextButton
    {
    public:
        enum Mode { Time, Note };
        Mode mode = Time;
        juce::Colour onColour  { juce::Colour::fromRGB (230, 176, 46) };
        juce::Colour offColour { juce::Colours::white.withAlpha (0.3f) };

        TimeNoteButton() { setClickingTogglesState (true); }

        void paintButton (juce::Graphics& g, bool isHighlighted, bool /*isDown*/) override
        {
            auto b = getLocalBounds().toFloat().reduced (1.0f);
            bool on = getToggleState();
            auto col = on ? onColour : offColour;
            if (isHighlighted) col = col.brighter (0.15f);

            if (on)
            {
                g.setColour (col.withAlpha (0.18f));
                g.fillRoundedRectangle (b, 3.0f);
                g.setColour (col.withAlpha (0.35f));
                g.drawRoundedRectangle (b, 3.0f, 0.8f);
            }

            g.setColour (col);

            if (mode == Time)
            {
                g.setFont (juce::FontOptions (9.5f));
                g.drawText ("ms", b, juce::Justification::centred, false);
            }
            else
            {
                // Draw a quarter-note icon
                float cx = b.getCentreX();
                float cy = b.getCentreY();
                g.fillEllipse (cx - 3.5f, cy + 1.0f, 6.0f, 4.5f);
                g.drawLine (cx + 2.5f, cy + 3.0f, cx + 2.5f, cy - 5.5f, 1.3f);
            }
        }
    };

    // --- Output EQ Frequency Response Graph Component ---
    class OutputEQGraph : public juce::Component
    {
    public:
        OutputEQGraph(juce::AudioProcessorValueTreeState& vts)
            : apvts(vts), bandMagnitudes(8)
        {
        }

        void setSampleRate(float sr) { sampleRate = sr; updateFrequencyResponse(); }

        void setAuroraState (float hue, float brightness)
        {
            auroraHue        = hue;
            auroraBrightness = brightness;
        }

        void updateFrequencyResponse()
        {
            if (sampleRate < 1000.0f)
                return;

            // Recompute all band magnitude curves
            std::vector<bool> enabledBands;
            for (int b = 0; b < 8; ++b)
            {
                bool isEnabled = false;
                if (auto* onParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("eqBand" + juce::String(b + 1) + "On")))
                    isEnabled = onParam->get();

                enabledBands.push_back(isEnabled);

                // Get current parameters for this band from APVTS
                float gainDb = 0.0f;
                float q = 0.707f;
                float freq = getBandFreq(b);

                if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("eqBand" + juce::String(b + 1) + "Gain")))
                    gainDb = p->get();
                if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("eqBand" + juce::String(b + 1) + "Q")))
                    q = p->get();

                // Compute filter coefficients for this band
                juce::dsp::IIR::Coefficients<float>::Ptr coeffs;
                float gain = juce::Decibels::decibelsToGain(gainDb);

                // Clamp frequency to avoid Nyquist issues
                freq = juce::jmin(freq, (float)sampleRate * 0.49f);

                switch (b)
                {
                    case 0: // HighPass 40Hz
                        coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq, q);
                        break;
                    case 1: // LowShelf 100Hz
                        coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, q, gain);
                        break;
                    case 2: case 3: case 4: case 5: // Bell filters
                        coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, gain);
                        break;
                    case 6: // HighShelf 10kHz
                        coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, freq, q, gain);
                        break;
                    case 7: // LowPass 16kHz
                        coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq, q);
                        break;
                    default:
                        break;
                }

                // Compute magnitude response for this band
                if (coeffs != nullptr)
                {
                    FrequencyResponseCalculator::fillFrequencyArray(*coeffs, bandMagnitudes[b], numFreqPoints, (float)sampleRate);
                }
                else
                {
                    bandMagnitudes[b].assign(numFreqPoints, 0.0f);
                }
            }

            // Compute sum curve
            FrequencyResponseCalculator::sumFrequencyArrays(bandMagnitudes, enabledBands, sumMagnitude);

            repaint();
        }

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds();
            if (area.isEmpty() || sampleRate < 1000.0f)
                return;

            // Grid padding and dimensions
            constexpr int gridPad = 40;
            constexpr int gridTopInset = 40;
            constexpr int gridBotInset = 20;
            constexpr int eqBtnStripH = 26;

            auto inner = area.reduced(gridPad, 0)
                .withTrimmedTop(gridTopInset + eqBtnStripH)
                .withTrimmedBottom(gridBotInset);

            // Background is filled by parent paint() method

            // --- Title ---
            bool eqMasterOn = false;
            if (auto* p = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("outputEqOn")))
                eqMasterOn = p->get();

            g.setColour(eqMasterOn ? juce::Colour::fromRGB(230, 176, 46) : juce::Colour::fromRGB(230, 176, 46).withAlpha(0.35f));
            g.setFont(juce::FontOptions(11.0f));
            g.drawText("OUTPUT EQ", inner.getX() + 22, area.getY() + 10, 120, 16,
                juce::Justification::centredLeft, false);

            // --- Aurora glow (behind everything) ---
            drawAuroraGlow (g, inner, auroraHue, auroraBrightness);

            // Draw horizontal grid lines (dB scale: -24 to +24)
            drawHorizontalGridLines(g, inner);

            // Draw vertical frequency lines
            drawFrequencyVerticalLines(g, inner);

            // Draw dB labels
            drawDBLabels(g, inner, gridPad);

            // Draw individual band curves
            drawBandCurves(g, inner);

            // Draw composite sum curve
            drawSumCurve(g, inner);

            // Draw control point handles
            drawHandles(g, inner);
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            auto area = getLocalBounds();
            constexpr int gridPad = 40;
            constexpr int gridTopInset = 40;
            constexpr int gridBotInset = 20;
            constexpr int eqBtnStripH = 26;

            auto inner = area.reduced(gridPad, 0)
                .withTrimmedTop(gridTopInset + eqBtnStripH)
                .withTrimmedBottom(gridBotInset);

            if (!inner.toFloat().contains(e.position))
                return;

            constexpr float hitRadius = 12.0f;

            // Check which band handle was clicked
            for (int i = 0; i < 8; ++i)
            {
                float nx = freqToNormX(getBandFreq(i));
                float bx = inner.getX() + nx * inner.getWidth();

                float gainDb = 0.0f;
                juce::String paramId = "eqBand" + juce::String(i + 1) + "Gain";
                if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramId)))
                    gainDb = p->get();

                float ny = (24.0f - gainDb) / 48.0f;
                float by = inner.getY() + ny * inner.getHeight();

                float dx = e.position.x - bx;
                float dy = e.position.y - by;
                
                if (dx * dx + dy * dy <= hitRadius * hitRadius)
                                {
                                    draggedBand = i;
                                    dragStartX = e.position.x;  // actual mouse click position
                                    dragStartY = e.position.y;  // actual mouse click position
                                    dragStartFreq = getBandFreq(i);
                                    if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("eqBand" + juce::String(i + 1) + "Gain")))
                                        dragStartGain = p->get();

                    // Determine drag mode based on click position
                    // HP/LP default to frequency drag, everything else defaults to gain drag
                                        // But all bands support both axes
                                        bool isHPorLP = (i == 0 || i == 7);
                                        if (isHPorLP)
                                            dragMode = std::abs(dy) > std::abs(dx) ? 2 : 2; // always frequency
                                        else
                                            dragMode = std::abs(dy) >= std::abs(dx) ? 1 : 2; // gain or frequency

                    if (auto* p = apvts.getParameter(paramId))
                                            p->beginChangeGesture();
                                        juce::String freqParamId = "eqBand" + juce::String(i + 1) + "Freq";
                                        if (auto* p = apvts.getParameter(freqParamId))
                                            p->beginChangeGesture();

                    return;
                }
            }
        }

        void mouseDrag(const juce::MouseEvent& e) override
                {
                    if (draggedBand < 0)
                        return;

                    auto area = getLocalBounds();
                    constexpr int gridPad = 40;
                    constexpr int gridTopInset = 40;
                    constexpr int gridBotInset = 20;
                    constexpr int eqBtnStripH = 26;

                    auto inner = area.reduced(gridPad, 0)
                        .withTrimmedTop(gridTopInset + eqBtnStripH)
                        .withTrimmedBottom(gridBotInset);

                    juce::String gainParamId = "eqBand" + juce::String(draggedBand + 1) + "Gain";
                    juce::String freqParamId = "eqBand" + juce::String(draggedBand + 1) + "Freq";

                    // X axis: frequency (delta based)
                    float dxPixels = e.position.x - dragStartX;
                    float logMin = std::log10(20.0f);
                    float logMax = std::log10(20000.0f);
                    float startLogFreq = std::log10(juce::jmax(dragStartFreq, 20.0f));
                    float newLogFreq = juce::jlimit(logMin, logMax, startLogFreq + (dxPixels / (float)inner.getWidth()) * (logMax - logMin));
                    float newFreq = std::pow(10.0f, newLogFreq);
                    if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(freqParamId)))
                    {
                        p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(newFreq));
                    }

                    
                              // Y axis: gain (delta from drag start)
                              float dyPixels = e.position.y - dragStartY;
                              float gainDelta = -(dyPixels / (float)inner.getHeight()) * 48.0f;
                              float newGain = juce::jlimit(-24.0f, 24.0f, dragStartGain + gainDelta);
                              if (auto* p = apvts.getParameter(gainParamId))
                                  p->setValueNotifyingHost((newGain + 24.0f) / 48.0f);
                    repaint();
                }

        void mouseUp(const juce::MouseEvent&) override
        {
            if (draggedBand >= 0)
            {
                juce::String gainParamId = "eqBand" + juce::String(draggedBand + 1) + "Gain";
                if (auto* p = apvts.getParameter(gainParamId))
                    p->endChangeGesture();

                juce::String freqParamId = "eqBand" + juce::String(draggedBand + 1) + "Freq";
                                if (auto* p = apvts.getParameter(freqParamId))
                                    p->endChangeGesture();

                draggedBand = -1;
                dragMode = 0;
            }

            repaint();
        }

        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
        {
            auto area = getLocalBounds();
            constexpr int gridPad = 40;
            constexpr int gridTopInset = 40;
            constexpr int gridBotInset = 20;
            constexpr int eqBtnStripH = 26;

            auto inner = area.reduced(gridPad, 0)
                .withTrimmedTop(gridTopInset + eqBtnStripH)
                .withTrimmedBottom(gridBotInset);

            constexpr float hitRadius = 12.0f;

            // Find if wheel is over a handle
            for (int i = 0; i < 8; ++i)
            {
                float nx = freqToNormX(getBandFreq(i));
                float bx = inner.getX() + nx * inner.getWidth();

                float gainDb = 0.0f;
                juce::String paramId = "eqBand" + juce::String(i + 1) + "Gain";
                if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramId)))
                    gainDb = p->get();

                float ny = (24.0f - gainDb) / 48.0f;
                float by = inner.getY() + ny * inner.getHeight();

                float dx = e.position.x - bx;
                float dy = e.position.y - by;

                if (dx * dx + dy * dy <= hitRadius * hitRadius)
                {
                    // Adjust Q parameter
                    juce::String qParamId = "eqBand" + juce::String(i + 1) + "Q";
                    if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(qParamId)))
                    {
                        float currentQ = p->get();
                        float newQ = currentQ + wheel.deltaY * 0.05f;
                        newQ = juce::jlimit(0.1f, 10.0f, newQ);
                        p->setValueNotifyingHost((newQ - 0.1f) / 9.9f); // Normalize to 0-1
                    }
                    return;
                }
            }
        }

    private:
        juce::AudioProcessorValueTreeState& apvts;
        std::vector<std::vector<float>> bandMagnitudes;
        std::vector<float> sumMagnitude;
        static constexpr int numFreqPoints = 512;
        float sampleRate = 44100.0f;

        // Default frequencies matching OutputEQ::bandFreqs
        static constexpr float defaultBandFreqs[8] = {
            40.0f, 100.0f, 250.0f, 800.0f, 2500.0f, 6000.0f, 10000.0f, 16000.0f
        };

        int draggedBand = -1;
                int dragMode = 0;
                float dragStartX = 0.0f;
                float dragStartY = 0.0f;
                float dragStartFreq = 0.0f;
                float dragStartGain = 0.0f;

        // Aurora glow state (pushed from ReverbPage timer)
        float auroraHue        = 0.0f;
        float auroraBrightness = 0.0f;

        void drawAuroraGlow (juce::Graphics& g, juce::Rectangle<int> inner,
                             float hue, float brightness)
        {
            if (brightness < 0.005f)
                return;

            const float hueOffsets[4]  = { 0.0f, 0.08f, 0.16f, 0.24f };
            const float xFractions[4]  = { 0.15f, 0.38f, 0.62f, 0.85f };
            const float yFraction      = 0.45f;

            for (int i = 0; i < 4; ++i)
            {
                float glowHue = hue + hueOffsets[i];
                if (glowHue > 1.0f) glowHue -= 1.0f;

                auto glowColour = juce::Colour::fromHSV (glowHue, 0.85f, 1.0f, 1.0f);

                float cx = inner.getX() + xFractions[i] * inner.getWidth();
                float cy = inner.getY() + yFraction * inner.getHeight();
                float radius = inner.getWidth() * 0.35f;

                juce::ColourGradient grad (
                    glowColour.withAlpha (brightness * 0.30f),
                    cx, cy,
                    glowColour.withAlpha (0.0f),
                    cx + radius, cy,
                    true
                );

                g.setGradientFill (grad);
                g.fillEllipse (cx - radius, cy - radius * 0.7f,
                               radius * 2.0f, radius * 1.4f);
            }
        }

        // Read band frequency from APVTS parameter (falls back to default if param missing)
        float getBandFreq(int bandIdx) const
        {
            juce::String paramId = "eqBand" + juce::String(bandIdx + 1) + "Freq";
            if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramId)))
                return p->get();
            return defaultBandFreqs[bandIdx];
        }

        float freqToNormX(float freq) const
        {
            float logMin = std::log10(20.0f);
            float logMax = std::log10(20000.0f);
            return (std::log10(juce::jmax(freq, 1.0f)) - logMin) / (logMax - logMin);
        }

        void drawHorizontalGridLines(juce::Graphics& g, juce::Rectangle<int> inner)
        {
            int numHLines = 8;
            for (int i = 0; i <= numHLines; ++i)
            {
                float y = inner.getY() + (inner.getHeight() * i / (float)numHLines);
                bool isZero = (i == numHLines / 2);

                if (isZero)
                    g.setColour(juce::Colours::white.withAlpha(0.22f));
                else
                    g.setColour(juce::Colours::white.withAlpha(0.08f));

                g.drawHorizontalLine(juce::roundToInt(y), (float)inner.getX(), (float)inner.getRight());
            }
        }

        void drawFrequencyVerticalLines(juce::Graphics& g, juce::Rectangle<int> inner)
        {
            const float freqs[] = { 20.0f, 30.0f, 40.0f, 60.0f, 80.0f, 100.0f, 200.0f, 300.0f, 500.0f,
                                   800.0f, 1000.0f, 2000.0f, 3000.0f, 5000.0f, 8000.0f, 10000.0f, 20000.0f };
            const int numFreqs = 17;

            for (int i = 0; i < numFreqs; ++i)
            {
                float nx = freqToNormX(freqs[i]);
                float x = inner.getX() + nx * inner.getWidth();

                bool isMajor = (freqs[i] == 100.0f || freqs[i] == 1000.0f || freqs[i] == 10000.0f);

                if (isMajor)
                    g.setColour(juce::Colours::white.withAlpha(0.12f));
                else
                    g.setColour(juce::Colours::white.withAlpha(0.07f));

                g.drawVerticalLine(juce::roundToInt(x), (float)inner.getY(), (float)inner.getBottom());
            }
        }

        void drawDBLabels(juce::Graphics& g, juce::Rectangle<int> inner, int gridPad)
        {
            g.setFont(juce::FontOptions(10.0f));
            const char* dbLabels[] = { "+24", "+18", "+12", "+6", "0", "-6", "-12", "-18", "-24" };
            g.setColour(juce::Colours::white.withAlpha(0.35f));

            for (int i = 0; i <= 8; ++i)
            {
                float y = inner.getY() + (inner.getHeight() * i / 8.0f);
                bool isZero = (i == 4);
                g.setColour(isZero ? juce::Colours::white.withAlpha(0.55f) : juce::Colours::white.withAlpha(0.35f));
                g.drawText(dbLabels[i], inner.getX() - gridPad, juce::roundToInt(y) - 7, gridPad - 4, 14,
                    juce::Justification::centredRight, false);
            }
        }

        void drawBandCurves(juce::Graphics& g, juce::Rectangle<int> inner)
        {
            juce::Colour bandColors[] = {
                juce::Colour::fromRGB(200, 140, 40),  // Band 0
                juce::Colour::fromRGB(180, 120, 35),  // Band 1
                juce::Colour::fromRGB(160, 100, 30),  // Band 2
                juce::Colour::fromRGB(140, 80, 25),   // Band 3
                juce::Colour::fromRGB(120, 60, 20),   // Band 4
                juce::Colour::fromRGB(100, 50, 15),   // Band 5
                juce::Colour::fromRGB(210, 150, 45),  // Band 6
                juce::Colour::fromRGB(190, 130, 40)   // Band 7
            };

            float zeroY = inner.getY() + inner.getHeight() * 0.5f;

            for (int b = 0; b < 8; ++b)
            {
                bool bandEnabled = true;
                if (auto* onParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("eqBand" + juce::String(b + 1) + "On")))
                    bandEnabled = onParam->get();

                if (!bandEnabled || bandMagnitudes[b].empty())
                    continue;

                juce::Path bandPath;
                bool firstPoint = true;

                for (int i = 0; i < (int)bandMagnitudes[b].size(); ++i)
                {
                    float t = i / (float)(bandMagnitudes[b].size() - 1);
                    float x = inner.getX() + t * inner.getWidth();
                    float magDb = bandMagnitudes[b][i];
                    float ny = (24.0f - magDb) / 48.0f;
                    float y = inner.getY() + ny * inner.getHeight();

                    if (firstPoint)
                    {
                        bandPath.startNewSubPath(x, y);
                        firstPoint = false;
                    }
                    else
                    {
                        bandPath.lineTo(x, y);
                    }
                }

                g.setColour(bandColors[b].withAlpha(0.5f));
                g.strokePath(bandPath, juce::PathStrokeType(1.2f));
            }
        }

        void drawSumCurve(juce::Graphics& g, juce::Rectangle<int> inner)
        {
            if (sumMagnitude.empty())
                return;

            bool eqMasterOn = false;
            if (auto* p = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("outputEqOn")))
                eqMasterOn = p->get();

            juce::Path sumPath;
            bool firstPoint = true;

            float zeroY = inner.getY() + inner.getHeight() * 0.5f;

            for (int i = 0; i < (int)sumMagnitude.size(); ++i)
            {
                float t = i / (float)(sumMagnitude.size() - 1);
                float x = inner.getX() + t * inner.getWidth();
                float magDb = sumMagnitude[i];
                float ny = (24.0f - magDb) / 48.0f;
                float y = inner.getY() + ny * inner.getHeight();

                if (firstPoint)
                {
                    sumPath.startNewSubPath(x, y);
                    firstPoint = false;
                }
                else
                {
                    sumPath.lineTo(x, y);
                }
            }

            // Fill between sum curve and 0dB line
            float zeroYFill = inner.getY() + inner.getHeight() * 0.5f;
            juce::Path fillPath(sumPath);
            fillPath.lineTo((float)inner.getRight(), zeroYFill);
            fillPath.lineTo((float)inner.getX(), zeroYFill);
            fillPath.closeSubPath();

            g.setColour(juce::Colour::fromRGB(230, 176, 46).withAlpha(eqMasterOn ? 0.10f : 0.03f));
            g.fillPath(fillPath);

            auto goldColour = juce::Colour::fromRGB (230, 176, 46);
            if (auroraBrightness > 0.05f && eqMasterOn)
            {
                auto auroraColour = juce::Colour::fromHSV (auroraHue, 0.9f, 1.0f, 1.0f);
                auto curveColour  = goldColour.interpolatedWith (auroraColour, auroraBrightness * 0.7f);
                g.setColour (curveColour.withAlpha (0.8f));
            }
            else
            {
                g.setColour (goldColour.withAlpha (eqMasterOn ? 0.8f : 0.2f));
            }
            g.strokePath(sumPath, juce::PathStrokeType(2.0f));
        }

        void drawHandles(juce::Graphics& g, juce::Rectangle<int> inner)
        {
            bool eqMasterOn = false;
            if (auto* p = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("outputEqOn")))
                eqMasterOn = p->get();

            for (int i = 0; i < 8; ++i)
            {
                bool bandOn = eqMasterOn;
                if (auto* onParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("eqBand" + juce::String(i + 1) + "On")))
                    bandOn = eqMasterOn && onParam->get();

                float nx = freqToNormX(getBandFreq(i));
                float bx = inner.getX() + nx * inner.getWidth();

                float gainDb = 0.0f;
                if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("eqBand" + juce::String(i + 1) + "Gain")))
                    gainDb = p->get();

                float ny = (24.0f - gainDb) / 48.0f;
                float by = inner.getY() + ny * inner.getHeight();

                bool isDragged = (draggedBand == i);
                float r = isDragged ? 7.0f : 5.0f;

                g.setColour(bandOn ? juce::Colour::fromRGB(230, 176, 46) : juce::Colour::fromRGB(230, 176, 46).withAlpha(0.15f));
                g.fillEllipse(bx - r, by - r, r * 2.0f, r * 2.0f);

                g.setColour(juce::Colours::black.withAlpha(0.4f));
                g.drawEllipse(bx - r, by - r, r * 2.0f, r * 2.0f, 1.0f);
            }
        }
    };

    ReverbPage (APOLLOAudioProcessor& p,
                juce::AudioProcessorValueTreeState& vts,
                std::atomic<float>& wetLevel)
        : audioProcessor (p), apvts (vts), wetLevelRef (wetLevel), currentPresetIndex (0)
    {
        // --- MAIN page knobs ---
        setupKnob (attackKnob,   attackLabel,   "Attack");
        setupKnob (sizeKnob,     sizeLabel,     "Size");
        setupKnob (densityKnob,  densityLabel,  "Density");
        setupKnob (decayKnob,    decayLabel,    "Decay");
        setupKnob (distanceKnob, distanceLabel, "Distance");
        setupKnob (predelayKnob, predelayLabel, "Predelay");

        setupFader (drySlider, dryLabel, "Dry");
        setupFader (wetSlider, wetLabel, "Wet");

        attackKnob.setTextValueSuffix (" %");
        sizeKnob.setTextValueSuffix (" %");
        densityKnob.setTextValueSuffix (" %");
        decayKnob.setTextValueSuffix (" s");
        distanceKnob.setTextValueSuffix (" %");
        predelayKnob.setTextValueSuffix (" ms");
        drySlider.setTextValueSuffix (" %");
        wetSlider.setTextValueSuffix (" %");

        freezeButton.setButtonText ("Freeze");
        freezeButton.setColour (juce::ToggleButton::textColourId, juce::Colours::white.withAlpha (0.8f));
        freezeButton.setColour (juce::ToggleButton::tickColourId, knobArc);
        addAndMakeVisible (freezeButton);

        // Decay range labels
        decayMinLabel.setText ("0.3", juce::dontSendNotification);
        decayMaxLabel.setText ("100", juce::dontSendNotification);
        for (auto* l : { &decayMinLabel, &decayMaxLabel })
        {
            l->setFont (juce::FontOptions (10.0f));
            l->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.35f));
            addAndMakeVisible (l);
        }
        decayMinLabel.setJustificationType (juce::Justification::centredRight);
        decayMaxLabel.setJustificationType (juce::Justification::centredLeft);

        // MAIN attachments
        attackAttach   = std::make_unique<SliderAttachment> (apvts, "attack",   attackKnob);
        sizeAttach     = std::make_unique<SliderAttachment> (apvts, "size",     sizeKnob);
        densityAttach  = std::make_unique<SliderAttachment> (apvts, "density",  densityKnob);
        decayAttach    = std::make_unique<SliderAttachment> (apvts, "decay",    decayKnob);
        distanceAttach = std::make_unique<SliderAttachment> (apvts, "distance", distanceKnob);
        predelayAttach = std::make_unique<SliderAttachment> (apvts, "predelay", predelayKnob);
        dryAttach      = std::make_unique<SliderAttachment> (apvts, "dry",      drySlider);
        wetAttach      = std::make_unique<SliderAttachment> (apvts, "wet",      wetSlider);
        freezeAttach   = std::make_unique<ButtonAttachment> (apvts, "freeze",   freezeButton);

        // --- DETAILS page knobs ---
        setupKnob (qualityKnob,   qualityLabel,   "Quality");
        setupKnob (modSpeedKnob,  modSpeedLabel,  "Mod Speed");
        setupKnob (modDepthKnob,  modDepthLabel,  "Mod Depth");
        setupKnob (smoothingKnob, smoothingLabel, "Smoothing");
        setupKnob (earlyLateKnob, earlyLateLabel, "Early/Late");
        setupKnob (widthKnob,     widthLabel,     "Width");
        setupKnob (monoMakerKnob, monoMakerLabel, "Mono Maker");

        qualityKnob.setTextValueSuffix (" %");
        modSpeedKnob.setTextValueSuffix (" Hz");
        modDepthKnob.setTextValueSuffix (" %");
        smoothingKnob.setTextValueSuffix (" %");
        earlyLateKnob.setTextValueSuffix (" %");
        widthKnob.setTextValueSuffix (" %");
        monoMakerKnob.setTextValueSuffix (" Hz");

        // --- Mod Source waveform buttons (DETAILS page) ---
        {
            const WaveformButton::WaveformType types[] = {
                WaveformButton::Sine, WaveformButton::Square, WaveformButton::Noise
            };
            for (int i = 0; i < 3; ++i)
            {
                modSourceBtns[i].waveType = types[i];
                modSourceBtns[i].setRadioGroupId (2001);
                addAndMakeVisible (modSourceBtns[i]);
                modSourceBtns[i].onClick = [this, i] { setModSourceParam (i); };
            }
        }
        modSourceLabel.setText ("Mod Source", juce::dontSendNotification);
        modSourceLabel.setJustificationType (juce::Justification::centred);
        modSourceLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        modSourceLabel.setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (modSourceLabel);

        // Set initial mod source state from parameter
        {
            auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("modSource"));
            int idx = p ? p->getIndex() : 0;
            modSourceBtns[idx].setToggleState (true, juce::dontSendNotification);
        }

        // DETAILS attachments
        qualityAttach   = std::make_unique<SliderAttachment> (apvts, "quality",   qualityKnob);
        modSpeedAttach  = std::make_unique<SliderAttachment> (apvts, "modSpeed",  modSpeedKnob);
        modDepthAttach  = std::make_unique<SliderAttachment> (apvts, "modDepth",  modDepthKnob);
        smoothingAttach = std::make_unique<SliderAttachment> (apvts, "smoothing", smoothingKnob);
        earlyLateAttach = std::make_unique<SliderAttachment> (apvts, "earlyLate", earlyLateKnob);
        widthAttach     = std::make_unique<SliderAttachment> (apvts, "width",     widthKnob);
        monoMakerAttach = std::make_unique<SliderAttachment> (apvts, "monoMaker", monoMakerKnob);

        // --- Output EQ master power button (DETAILS page) ---
        addAndMakeVisible (outputEqButton);
        outputEqButton.onClick = [this] { repaint(); };
        outputEqAttach = std::make_unique<ButtonAttachment> (apvts, "outputEqOn", outputEqButton);

        // --- EQ band shape buttons (DETAILS page) ---
        {
            const EQBandButton::FilterType types[] = {
                EQBandButton::HighPass,  EQBandButton::LowShelf,
                EQBandButton::Bell,      EQBandButton::Bell,
                EQBandButton::Bell,      EQBandButton::Bell,
                EQBandButton::HighShelf, EQBandButton::LowPass
            };

            for (int i = 0; i < 8; ++i)
            {
                eqBandButtons[i].filterType = types[i];
                addAndMakeVisible (eqBandButtons[i]);
                eqBandButtons[i].onClick = [this] { repaint(); };
                juce::String paramId = "eqBand" + juce::String (i + 1) + "On";
                eqBandAttachments[i] = std::make_unique<ButtonAttachment> (apvts, paramId, eqBandButtons[i]);
            }
        }

        // --- Mono Maker power button (DETAILS page) ---
        addAndMakeVisible (monoMakerPowerBtn);
        monoMakerPowerBtn.onClick = [this] { repaint(); };
        monoMakerPowerAttach = std::make_unique<ButtonAttachment> (apvts, "monoMakerOn", monoMakerPowerBtn);

        // --- Sub-tab buttons ---
        setupSubTabButton (mainButton, true);
        setupSubTabButton (detailsButton, false);

        mainButton.onClick    = [this] { switchSubPage (0); };
        detailsButton.onClick = [this] { switchSubPage (1); };

        // --- Master reverb power button (top strip) ---
        addAndMakeVisible (masterPowerBtn);
        masterPowerBtn.onClick = [this] { repaint(); };
        masterPowerAttach = std::make_unique<ButtonAttachment> (apvts, "reverbOn", masterPowerBtn);

        // --- Room type selector (top strip) ---
        {
            const juce::StringArray roomNames { "Room", "Chamber", "Concert Hall", "Theatre",
                                                 "Synth Hall", "Digital", "Dark Room", "Dense",
                                                 "Smooth Space", "Vocal Hall", "Reflective Hall",
                                                 "Strange Room", "Airy", "Bloomy" };
            for (int i = 0; i < roomNames.size(); ++i)
                roomSelector.addItem (roomNames[i], i + 1);
        }
        roomSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (45, 47, 52));
        roomSelector.setColour (juce::ComboBox::textColourId, juce::Colours::white);
        roomSelector.setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.15f));
        roomSelector.setColour (juce::ComboBox::arrowColourId, knobArc);
        roomSelector.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (roomSelector);
        roomAttach = std::make_unique<ComboBoxAttachment> (apvts, "roomType", roomSelector);

        // --- Preset strip components ---
        presetCategoryButton.setButtonText ("Factory Presets");
        presetCategoryButton.setColour (juce::TextButton::buttonColourId, presetBtnColour);
        presetCategoryButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.85f));
        addAndMakeVisible (presetCategoryButton);

        presetNameLabel.setText ("Default", juce::dontSendNotification);
        presetNameLabel.setJustificationType (juce::Justification::centred);
        presetNameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        presetNameLabel.setColour (juce::Label::backgroundColourId, presetNameBg);
        presetNameLabel.setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (presetNameLabel);

        prevPresetButton.setButtonText ("<");
        prevPresetButton.setColour (juce::TextButton::buttonColourId, presetBtnColour);
        prevPresetButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.85f));
        addAndMakeVisible (prevPresetButton);

        nextPresetButton.setButtonText (">");
        nextPresetButton.setColour (juce::TextButton::buttonColourId, presetBtnColour);
        nextPresetButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.85f));
        addAndMakeVisible (nextPresetButton);

        savePresetButton.setButtonText ("Save");
        savePresetButton.setColour (juce::TextButton::buttonColourId, presetBtnColour);
        savePresetButton.setColour (juce::TextButton::textColourOffId, knobArc);
        addAndMakeVisible (savePresetButton);

        // --- Preset button handlers ---
        prevPresetButton.onClick = [this] { cyclePreset (-1); };
        nextPresetButton.onClick = [this] { cyclePreset (1); };
        presetCategoryButton.onClick = [this] { showPresetMenu(); };

        // Load initial preset (savedPresetIndex[0] encodes flat index across categories)
        {
            int savedIdx = juce::jlimit (0, totalReverbPresets - 1, audioProcessor.savedPresetIndex[0]);
            flatIndexToCategory (savedIdx, currentCategory, currentPresetIndex);
        }
        updatePresetName();

        // --- Decay mode buttons (Time / Note) ---
        {
            decayModeBtns[0].mode = TimeNoteButton::Time;
            decayModeBtns[1].mode = TimeNoteButton::Note;
            for (int i = 0; i < 2; ++i)
            {
                decayModeBtns[i].setRadioGroupId (3001);
                addAndMakeVisible (decayModeBtns[i]);
                decayModeBtns[i].onClick = [this, i] { setDecayMode (i); };
            }
        }

        // --- Predelay mode buttons (Time / Note) ---
        {
            predelayModeBtns[0].mode = TimeNoteButton::Time;
            predelayModeBtns[1].mode = TimeNoteButton::Note;
            for (int i = 0; i < 2; ++i)
            {
                predelayModeBtns[i].setRadioGroupId (3002);
                addAndMakeVisible (predelayModeBtns[i]);
                predelayModeBtns[i].onClick = [this, i] { setPredelayMode (i); };
            }
        }

        // Set initial mode states from parameters
        {
            auto* dp = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("decayMode"));
            int dIdx = dp ? dp->getIndex() : 0;
            decayModeBtns[dIdx].setToggleState (true, juce::dontSendNotification);
        }
        {
            auto* pp = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("predelayMode"));
            int pIdx = pp ? pp->getIndex() : 0;
            predelayModeBtns[pIdx].setToggleState (true, juce::dontSendNotification);
        }

        // Start on MAIN sub-page
        switchSubPage (0);

        // --- Initialize EQ frequency response graph ---
        eqGraph = std::make_unique<OutputEQGraph> (apvts);
        eqGraph->setOpaque (false);
        addAndMakeVisible (*eqGraph);
        eqGraph->toBack();

        // Register parameter listeners
        for (auto* id : { "size", "density", "attack", "distance", "decay", "roomType" })
            apvts.addParameterListener (id, this);
        apvts.addParameterListener ("outputEqOn", this);

        for (int i = 1; i <= 5; ++i)
            apvts.addParameterListener ("dampDotFreq" + juce::String (i), this);

        for (int i = 1; i <= 8; ++i)
        {
            apvts.addParameterListener ("eqBand" + juce::String (i) + "On", this);
            apvts.addParameterListener ("eqBand" + juce::String (i) + "Gain", this);
            apvts.addParameterListener ("eqBand" + juce::String (i) + "Q", this);
            apvts.addParameterListener ("eqBand" + juce::String (i) + "Freq", this);
        }

        // Initial frequency response update
        eqGraph->updateFrequencyResponse();

        // Start 60fps aurora glow timer
        startTimerHz (60);
    }

    ~ReverbPage() override
    {
        stopTimer();
        for (auto* id : { "size", "density", "attack", "distance", "decay", "roomType" })
            apvts.removeParameterListener (id, this);
        apvts.removeParameterListener ("outputEqOn", this);

        for (int i = 1; i <= 5; ++i)
            apvts.removeParameterListener ("dampDotFreq" + juce::String (i), this);

        for (int i = 1; i <= 8; ++i)
        {
            apvts.removeParameterListener ("eqBand" + juce::String (i) + "On", this);
            apvts.removeParameterListener ("eqBand" + juce::String (i) + "Gain", this);
            apvts.removeParameterListener ("eqBand" + juce::String (i) + "Q", this);
            apvts.removeParameterListener ("eqBand" + juce::String (i) + "Freq", this);
        }
    }

    void parameterChanged (const juce::String& paramId, float) override
    {
        if (eqGraph && (paramId.startsWith ("eqBand") || paramId == "outputEqOn"))
            eqGraph->updateFrequencyResponse();

        repaint();
    }

    // =========================================================================
    // Mouse interaction for EQ dots
    // =========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (currentSubPage != 0)  // only active on MAIN page
            return;

        auto inner = getDampingEQInner();
        if (!inner.toFloat().contains (e.position))
            return;

        constexpr float hitRadius = 12.0f;
        auto dots = computeDampingDotPositions (inner);

        // Dots 0-4 are interactive (0-based indexing)
        for (int i = 0; i < 5; ++i)
        {
            float dx = e.position.x - dots[(size_t) i].x;
            float dy = e.position.y - dots[(size_t) i].y;

            if (dx * dx + dy * dy <= hitRadius * hitRadius)
            {
                draggedDampingDot  = i;
                dampDragStartX     = e.position.x;
                dampDragStartY     = e.position.y;

                // Store starting freq from parameter
                juce::String freqId = "dampDotFreq" + juce::String (i + 1);
                if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                        apvts.getParameter (freqId)))
                    dampDragStartFreq = p->get();

                // Store starting Y param value (normalised 0-1)
                const char* yParamId = dampingDotParamId (i + 1);
                if (yParamId != nullptr)
                {
                    if (auto* p = apvts.getParameter (yParamId))
                    {
                        dampDragStartParam = p->getValue();
                        p->beginChangeGesture();
                    }
                    if (auto* p = apvts.getParameter (freqId))
                        p->beginChangeGesture();
                }
                return;
            }
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (draggedDampingDot < 0 || currentSubPage != 0)
            return;

        auto inner = getDampingEQInner();
        int i = draggedDampingDot;

        // --- X axis: shift dot frequency (delta-based, log scale) ---
        {
            float dxPixels = e.position.x - dampDragStartX;
            float logMin   = std::log10 (20.0f);
            float logMax   = std::log10 (20000.0f);
            float startLog = std::log10 (juce::jmax (dampDragStartFreq, 20.0f));
            float newLog   = juce::jlimit (logMin, logMax,
                                 startLog + (dxPixels / (float) inner.getWidth())
                                            * (logMax - logMin));
            float newFreq  = std::pow (10.0f, newLog);

            juce::String freqId = "dampDotFreq" + juce::String (i + 1);
            if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    apvts.getParameter (freqId)))
            {
                p->setValueNotifyingHost (
                    p->getNormalisableRange().convertTo0to1 (newFreq));
            }
        }

        // --- Y axis: adjust linked reverb parameter (delta-based) ---
        {
            const char* yParamId = dampingDotParamId (i + 1);
            if (yParamId != nullptr)
            {
                float dyPixels = e.position.y - dampDragStartY;
                // Dragging UP = increase param (invert dy)
                float delta    = -(dyPixels / (float) inner.getHeight());
                float newVal   = juce::jlimit (0.0f, 1.0f,
                                     dampDragStartParam + delta);
                if (auto* p = apvts.getParameter (yParamId))
                    p->setValueNotifyingHost (newVal);
            }
        }

        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (draggedDampingDot >= 0)
        {
            const char* yParamId = dampingDotParamId (draggedDampingDot + 1);
            if (yParamId != nullptr)
                if (auto* p = apvts.getParameter (yParamId))
                    p->endChangeGesture();

            juce::String freqId = "dampDotFreq" + juce::String (draggedDampingDot + 1);
            if (auto* p = apvts.getParameter (freqId))
                p->endChangeGesture();

            draggedDampingDot = -1;
            repaint();
        }
    }

    void mouseMove (const juce::MouseEvent&) override
    {
        // eqGraph handles cursor changes via its own mouse handlers
    }

    void timerCallback() override
    {
        // Advance hue slowly regardless of level (always alive)
        auroraHue += 0.004f;
        if (auroraHue > 1.0f) auroraHue -= 1.0f;

        // Read wet level from audio thread
        float rawLevel = wetLevelRef.load (std::memory_order_relaxed);

        // Smooth brightness toward target level
        // Fast attack (0.4), slow release (0.08) for punchy feel
        if (rawLevel > auroraBrightness)
            auroraBrightness += (rawLevel - auroraBrightness) * 0.4f;
        else
            auroraBrightness += (rawLevel - auroraBrightness) * 0.08f;

        auroraBrightness = juce::jlimit (0.0f, 1.0f, auroraBrightness);

        // Push aurora state to the Details graph component
        if (eqGraph)
            eqGraph->setAuroraState (auroraHue, auroraBrightness);

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // --- Preset strip background ---
        {
            auto topStrip = bounds.removeFromTop (topStripH);
            g.setColour (topStripBg);
            g.fillRect (topStrip);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawHorizontalLine (topStrip.getBottom() - 1,
                                  (float) topStrip.getX(), (float) topStrip.getRight());
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.drawHorizontalLine (topStrip.getBottom(),
                                  (float) topStrip.getX(), (float) topStrip.getRight());
        }

        int gridH = juce::roundToInt (bounds.getHeight() * gridProportion);
        auto gridArea    = bounds.removeFromTop (gridH);
        auto controlArea = bounds;

        // --- Grid display ---
        g.setColour (gridBackground);
        g.fillRect (gridArea);

        if (currentSubPage == 0)
                    drawDampingEQ (g, gridArea);
                // DETAILS page: eqGraph child component draws itself
        
        // --- Bottom: silver control panel ---
        juce::ColourGradient silverGrad (silverTop, 0.0f, (float) controlArea.getY(),
                                         silverBottom, 0.0f, (float) controlArea.getBottom(), false);
        g.setGradientFill (silverGrad);
        g.fillRect (controlArea);

        // Divider line at top
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawHorizontalLine (controlArea.getY(), (float) controlArea.getX(),
                              (float) controlArea.getRight());

        // --- Vertical separators between parameter groups ---
        auto ctrlInner = controlArea.reduced (20, 0);
        int totalW = ctrlInner.getWidth();

        float sepTop = (float) (controlArea.getY() + 16);
        float sepBot = (float) (controlArea.getBottom() - 16);

        auto drawSeparator = [&] (int sx)
        {
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.drawVerticalLine (sx, sepTop, sepBot);
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.drawVerticalLine (sx + 1, sepTop, sepBot);
        };

        if (currentSubPage == 0)
        {
            int s1W = juce::roundToInt (totalW * 0.35f);
            int s2W = juce::roundToInt (totalW * 0.16f);
            int s3W = juce::roundToInt (totalW * 0.26f);

            drawSeparator (ctrlInner.getX() + s1W);
            drawSeparator (ctrlInner.getX() + s1W + s2W);
            drawSeparator (ctrlInner.getX() + s1W + s2W + s3W);
        }
        else
        {
            int d1W  = juce::roundToInt (totalW * 0.36f);
            int msW  = juce::roundToInt (totalW * 0.08f);
            int d2W  = juce::roundToInt (totalW * 0.28f);

            drawSeparator (ctrlInner.getX() + d1W);
            drawSeparator (ctrlInner.getX() + d1W + msW);
            drawSeparator (ctrlInner.getX() + d1W + msW + d2W);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // --- Preset strip layout (always visible) ---
        {
            auto presetArea = bounds.removeFromTop (topStripH).reduced (12, 5);
            int presetBtnH = presetArea.getHeight();

            masterPowerBtn.setBounds (presetArea.removeFromLeft (presetBtnH).withHeight (presetBtnH));
            presetArea.removeFromLeft (6);
            presetCategoryButton.setBounds (presetArea.removeFromLeft (130).withHeight (presetBtnH));
            presetArea.removeFromLeft (8);

            savePresetButton.setBounds (presetArea.removeFromRight (50).withHeight (presetBtnH));
            presetArea.removeFromRight (6);
            nextPresetButton.setBounds (presetArea.removeFromRight (28).withHeight (presetBtnH));
            presetArea.removeFromRight (2);
            prevPresetButton.setBounds (presetArea.removeFromRight (28).withHeight (presetBtnH));
            presetArea.removeFromRight (8);

            // Position preset name centered above where Room dropdown will be
            int labelW = 170;
            int labelX = (getWidth() / 2) - (labelW / 2);
            presetNameLabel.setBounds (labelX, presetArea.getY(), labelW, presetBtnH);
        }

        int gridH = juce::roundToInt (bounds.getHeight() * gridProportion);

        // --- Sub-tab buttons in upper-right of grid area ---
        int btnW = 65, btnH = 24, btnGap = 1, btnMarginR = 12, btnMarginT = 8;
        mainButton.setBounds (bounds.getRight() - btnMarginR - 2 * btnW - btnGap,
                              bounds.getY() + btnMarginT,
                              btnW, btnH);
        detailsButton.setBounds (mainButton.getRight() + btnGap,
                                 bounds.getY() + btnMarginT,
                                 btnW, btnH);

        // --- Room selector — centered in grid area ---
        {
            int rsBtnW = 170, rsBtnH = 26;
            int rsX = bounds.getCentreX() - rsBtnW / 2;
            int rsY = bounds.getY() + btnMarginT + 4;
            roomSelector.setBounds (rsX, rsY, rsBtnW, rsBtnH);
        }

        // --- EQ Graph (shown on both pages) ---
        if (eqGraph)
                {
                    auto gridArea = bounds.withHeight (gridH);
                    eqGraph->setBounds (gridArea);
                    eqGraph->setVisible (currentSubPage == 1);
                }
        
        auto controlArea = bounds.withTrimmedTop (gridH).reduced (20, 15);

        int knobW = 105;
        int knobH = 95;
        int labelH = 18;
        int faderW = 55;

        int totalW = controlArea.getWidth();
        int rowH = labelH + knobH;

        if (currentSubPage == 0)
        {
            int sec1W = juce::roundToInt (totalW * 0.35f);
            int sec2W = juce::roundToInt (totalW * 0.16f);
            int sec3W = juce::roundToInt (totalW * 0.26f);
            int sec4W = totalW - sec1W - sec2W - sec3W;

            int sec1X = controlArea.getX();
            int sec2X = sec1X + sec1W;
            int sec3X = sec2X + sec2W;
            int sec4X = sec3X + sec3W;

            int rowY = controlArea.getY() + (controlArea.getHeight() - rowH) / 2 - 14;

            {
                int groupW = 3 * knobW;
                int gap = (sec1W - groupW) / 4;
                int x = sec1X + gap;
                layoutKnob (attackLabel,   attackKnob,   x, rowY, knobW, labelH, knobH);
                x += knobW + gap;
                layoutKnob (sizeLabel,     sizeKnob,     x, rowY, knobW, labelH, knobH);
                x += knobW + gap;
                layoutKnob (densityLabel,  densityKnob,  x, rowY, knobW, labelH, knobH);
            }

            {
                int x = sec2X + (sec2W - knobW) / 2;
                layoutKnob (decayLabel, decayKnob, x, rowY, knobW, labelH, knobH);
                freezeButton.setBounds (x + 5, rowY + labelH + knobH + 4, knobW - 10, 20);
                decayMinLabel.setBounds (x - 28, rowY + labelH + knobH + 4, 26, 14);
                decayMaxLabel.setBounds (x + knobW + 2, rowY + labelH + knobH + 4, 26, 14);

                // Decay mode buttons (Time / Note) — above label
                int mbW = 22, mbH = 18, mbGap = 2;
                int totalMbW = 2 * mbW + mbGap;
                int mbX = x + (knobW - totalMbW) / 2;
                int mbY = rowY - mbH - 2;
                decayModeBtns[0].setBounds (mbX, mbY, mbW, mbH);
                decayModeBtns[1].setBounds (mbX + mbW + mbGap, mbY, mbW, mbH);
            }

            {
                int groupW = 2 * knobW;
                int gap = (sec3W - groupW) / 3;
                int x = sec3X + gap;
                layoutKnob (distanceLabel, distanceKnob, x, rowY, knobW, labelH, knobH);
                x += knobW + gap;
                layoutKnob (predelayLabel, predelayKnob, x, rowY, knobW, labelH, knobH);

                // Predelay mode buttons (Time / Note) — above label
                int mbW = 22, mbH = 18, mbGap = 2;
                int totalMbW = 2 * mbW + mbGap;
                int mbX = x + (knobW - totalMbW) / 2;
                int mbY = rowY - mbH - 2;
                predelayModeBtns[0].setBounds (mbX, mbY, mbW, mbH);
                predelayModeBtns[1].setBounds (mbX + mbW + mbGap, mbY, mbW, mbH);
            }

            {
                int faderH = rowH + 30;
                int gap = (sec4W - 2 * faderW) / 3;
                int x = sec4X + gap;
                layoutFader (dryLabel, drySlider, x, rowY, faderW, labelH, faderH);
                x += faderW + gap;
                layoutFader (wetLabel, wetSlider, x, rowY, faderW, labelH, faderH);
            }
        }
        else
        {
            int dSec1W  = juce::roundToInt (totalW * 0.36f);
            int modSrcW = juce::roundToInt (totalW * 0.08f);
            int dSec2W  = juce::roundToInt (totalW * 0.28f);
            int dSec3W  = totalW - dSec1W - modSrcW - dSec2W;

            int dSec1X  = controlArea.getX();
            int modSrcX = dSec1X + dSec1W;
            int dSec2X  = modSrcX + modSrcW;
            int dSec3X  = dSec2X + dSec2W;

            int rowY = controlArea.getY() + (controlArea.getHeight() - rowH) / 2 - 6;

            // --- Section 1: Quality, Mod Speed, Mod Depth ---
            {
                int groupW = 3 * knobW;
                int gap = (dSec1W - groupW) / 4;
                int x = dSec1X + gap;
                layoutKnob (qualityLabel,  qualityKnob,  x, rowY, knobW, labelH, knobH);
                x += knobW + gap;
                layoutKnob (modSpeedLabel, modSpeedKnob, x, rowY, knobW, labelH, knobH);
                x += knobW + gap;
                layoutKnob (modDepthLabel, modDepthKnob, x, rowY, knobW, labelH, knobH);
            }

            // --- Mod Source waveform buttons ---
            {
                int btnSize = 26;
                int btnGap = 3;
                int totalBtnsW = 3 * btnSize + 2 * btnGap;
                int bx = modSrcX + (modSrcW - totalBtnsW) / 2;
                modSourceLabel.setBounds (modSrcX, rowY, modSrcW, labelH);
                int by = rowY + labelH + (knobH - btnSize) / 2;
                for (int i = 0; i < 3; ++i)
                    modSourceBtns[i].setBounds (bx + i * (btnSize + btnGap), by, btnSize, btnSize);
            }

            // --- Section 2: Smoothing, Early/Late ---
            {
                int groupW = 2 * knobW;
                int gap = (dSec2W - groupW) / 3;
                int x = dSec2X + gap;
                layoutKnob (smoothingLabel, smoothingKnob, x, rowY, knobW, labelH, knobH);
                x += knobW + gap;
                layoutKnob (earlyLateLabel, earlyLateKnob, x, rowY, knobW, labelH, knobH);
            }

            // --- Section 3: Width, Mono Maker ---
            {
                int groupW = 2 * knobW;
                int gap = (dSec3W - groupW) / 3;
                int x = dSec3X + gap;
                layoutKnob (widthLabel,     widthKnob,     x, rowY, knobW, labelH, knobH);
                x += knobW + gap;
                layoutKnob (monoMakerLabel, monoMakerKnob, x, rowY, knobW, labelH, knobH);

                // Mono Maker power button — to the left of the label
                int mmPwrSize = 16;
                monoMakerPowerBtn.setBounds (x - mmPwrSize - 2,
                                             rowY + (labelH - mmPwrSize) / 2,
                                             mmPwrSize, mmPwrSize);
            }

        }

        // --- Position master Output EQ power button next to title (both pages) ---
        {
            auto eqGridArea = getGridArea();
            int masterBtnSize = 18;
            int titleX = eqGridArea.getX() + gridPad;
            int titleY = eqGridArea.getY() + 9;
            outputEqButton.setBounds (titleX, titleY, masterBtnSize, masterBtnSize);
        }

        // --- Position EQ band shape buttons in strip above output EQ grid (both pages) ---
        {
            auto eqGridArea = getGridArea();
            auto eqInner = eqGridArea.reduced (gridPad, 0)
                .withTrimmedTop (gridTopInset + eqBtnStripH)
                .withTrimmedBottom (gridBotInset);

            int btnSize = 22;
            int stripY = eqGridArea.getY() + gridTopInset;

            for (int i = 0; i < 8; ++i)
            {
                float nx = freqToNormX (eqBandFreqs[i]);
                int bx = eqInner.getX() + juce::roundToInt (nx * eqInner.getWidth());
                eqBandButtons[i].setBounds (bx - btnSize / 2, stripY, btnSize, btnSize);
            }
        }
    }

private:
    // =========================================================================
    // Helpers
    // =========================================================================
    void setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& name)
    {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
        knob.setColour (juce::Slider::rotarySliderFillColourId, knobArc);
        knob.setColour (juce::Slider::rotarySliderOutlineColourId, knobTrack);
        knob.setColour (juce::Slider::thumbColourId, knobArc);
        knob.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        knob.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (knob);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colours::white);
        label.setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (label);
    }

    void setupFader (juce::Slider& fader, juce::Label& label, const juce::String& name)
    {
        fader.setSliderStyle (juce::Slider::LinearVertical);
        fader.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 50, 16);
        fader.setColour (juce::Slider::trackColourId, knobArc);
        fader.setColour (juce::Slider::thumbColourId, juce::Colours::white);
        fader.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        fader.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (fader);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colours::white);
        label.setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (label);
    }

    void setupSubTabButton (juce::TextButton& btn, bool isActive)
    {
        btn.setClickingTogglesState (false);
        btn.setColour (juce::TextButton::buttonColourId,
                       isActive ? subTabActive : subTabInactive);
        btn.setColour (juce::TextButton::textColourOffId,
                       isActive ? knobArc : juce::Colours::white.withAlpha (0.5f));
        btn.setColour (juce::TextButton::buttonOnColourId, subTabActive);
        btn.setColour (juce::TextButton::textColourOnId, knobArc);
        addAndMakeVisible (btn);
    }

    void switchSubPage (int subPage)
    {
        currentSubPage = subPage;

        bool showMain    = (subPage == 0);
        bool showDetails = (subPage == 1);

        attackKnob.setVisible (showMain);     attackLabel.setVisible (showMain);
        sizeKnob.setVisible (showMain);       sizeLabel.setVisible (showMain);
        densityKnob.setVisible (showMain);    densityLabel.setVisible (showMain);
        decayKnob.setVisible (showMain);      decayLabel.setVisible (showMain);
        distanceKnob.setVisible (showMain);   distanceLabel.setVisible (showMain);
        predelayKnob.setVisible (showMain);   predelayLabel.setVisible (showMain);
        drySlider.setVisible (showMain);      dryLabel.setVisible (showMain);
        wetSlider.setVisible (showMain);      wetLabel.setVisible (showMain);
        freezeButton.setVisible (showMain);
        decayMinLabel.setVisible (showMain);
        decayMaxLabel.setVisible (showMain);

        qualityKnob.setVisible (showDetails);    qualityLabel.setVisible (showDetails);
        modSpeedKnob.setVisible (showDetails);   modSpeedLabel.setVisible (showDetails);
        modDepthKnob.setVisible (showDetails);   modDepthLabel.setVisible (showDetails);
        smoothingKnob.setVisible (showDetails);  smoothingLabel.setVisible (showDetails);
        earlyLateKnob.setVisible (showDetails);  earlyLateLabel.setVisible (showDetails);
        widthKnob.setVisible (showDetails);      widthLabel.setVisible (showDetails);
        monoMakerKnob.setVisible (showDetails);  monoMakerLabel.setVisible (showDetails);

        modSourceLabel.setVisible (showDetails);
        for (int i = 0; i < 3; ++i)
            modSourceBtns[i].setVisible (showDetails);

        // Preset strip is always visible
        masterPowerBtn.setVisible (true);
        presetCategoryButton.setVisible (true);
        presetNameLabel.setVisible (true);
        prevPresetButton.setVisible (true);
        nextPresetButton.setVisible (true);
        savePresetButton.setVisible (true);

        // Room selector in grid area — MAIN only
        roomSelector.setVisible (showMain);

        for (int i = 0; i < 2; ++i)
        {
            decayModeBtns[i].setVisible (showMain);
            predelayModeBtns[i].setVisible (showMain);
        }

        monoMakerPowerBtn.setVisible (showDetails);

        // EQ buttons only on DETAILS page
                outputEqButton.setVisible (showDetails);
                for (int i = 0; i < 8; ++i)
                    eqBandButtons[i].setVisible (showDetails);

        mainButton.setColour (juce::TextButton::buttonColourId,
                              showMain ? subTabActive : subTabInactive);
        mainButton.setColour (juce::TextButton::textColourOffId,
                              showMain ? knobArc : juce::Colours::white.withAlpha (0.5f));

        detailsButton.setColour (juce::TextButton::buttonColourId,
                                 showDetails ? subTabActive : subTabInactive);
        detailsButton.setColour (juce::TextButton::textColourOffId,
                                 showDetails ? knobArc : juce::Colours::white.withAlpha (0.5f));

        resized();
        repaint();
    }

    void setModSourceParam (int index)
    {
        if (auto* p = apvts.getParameter ("modSource"))
        {
            float normValue = (index == 0) ? 0.0f : (index == 1) ? 0.5f : 1.0f;
            p->beginChangeGesture();
            p->setValueNotifyingHost (normValue);
            p->endChangeGesture();
        }
    }

    // --- Note name helpers (assumes 120 BPM default) ---
    static juce::String secondsToNoteName (double seconds)
    {
        struct NoteVal { float dur; const char* name; };
        static const NoteVal notes[] = {
            { 0.0625f,  "1/32" }, { 0.09375f, "1/32." },
            { 0.125f,   "1/16" }, { 0.1875f,  "1/16." },
            { 0.25f,    "1/8"  }, { 0.375f,   "1/8."  },
            { 0.5f,     "1/4"  }, { 0.75f,    "1/4."  },
            { 1.0f,     "1/2"  }, { 1.5f,     "1/2."  },
            { 2.0f,     "1/1"  }, { 3.0f,     "1/1."  },
            { 4.0f,     "2/1"  }, { 6.0f,     "2/1."  },
            { 8.0f,     "4/1"  }, { 16.0f,    "8/1"   },
            { 32.0f,    "16/1" }, { 64.0f,    "32/1"  }
        };
        static const int numNotes = 18;

        float minDist = 9999.0f;
        int closest = 0;
        for (int i = 0; i < numNotes; ++i)
        {
            float d = std::abs ((float) seconds - notes[i].dur);
            if (d < minDist) { minDist = d; closest = i; }
        }
        return notes[closest].name;
    }

    static juce::String msToNoteName (double ms)
    {
        struct NoteVal { float dur; const char* name; };
        static const NoteVal notes[] = {
            { 0.0f,     "0"     },
            { 15.625f,  "1/128" }, { 23.4f, "1/128." },
            { 31.25f,   "1/64"  }, { 46.9f, "1/64."  },
            { 62.5f,    "1/32"  }, { 93.8f, "1/32."  },
            { 125.0f,   "1/16"  }, { 187.5f, "1/16." },
            { 250.0f,   "1/8"   }
        };
        static const int numNotes = 10;

        float minDist = 9999.0f;
        int closest = 0;
        for (int i = 0; i < numNotes; ++i)
        {
            float d = std::abs ((float) ms - notes[i].dur);
            if (d < minDist) { minDist = d; closest = i; }
        }
        return notes[closest].name;
    }

    void setDecayMode (int index)
    {
        if (auto* p = apvts.getParameter ("decayMode"))
        {
            float normValue = (index == 0) ? 0.0f : 1.0f;
            p->beginChangeGesture();
            p->setValueNotifyingHost (normValue);
            p->endChangeGesture();
        }

        if (index == 0) // Time
        {
            decayKnob.textFromValueFunction = nullptr;
            decayKnob.setTextValueSuffix (" s");
        }
        else // Note
        {
            decayKnob.textFromValueFunction = [] (double val)
            {
                return secondsToNoteName (val);
            };
            decayKnob.setTextValueSuffix ("");
        }
        decayKnob.updateText();
    }

    void setPredelayMode (int index)
    {
        if (auto* p = apvts.getParameter ("predelayMode"))
        {
            float normValue = (index == 0) ? 0.0f : 1.0f;
            p->beginChangeGesture();
            p->setValueNotifyingHost (normValue);
            p->endChangeGesture();
        }

        if (index == 0) // Time
        {
            predelayKnob.textFromValueFunction = nullptr;
            predelayKnob.setTextValueSuffix (" ms");
        }
        else // Note
        {
            predelayKnob.textFromValueFunction = [] (double val)
            {
                return msToNoteName (val);
            };
            predelayKnob.setTextValueSuffix ("");
        }
        predelayKnob.updateText();
    }

    void cyclePreset (int direction)
    {
        int numInCategory = presetsPerCategory[currentCategory];
        currentPresetIndex += direction;
        if (currentPresetIndex < 0) currentPresetIndex = numInCategory - 1;
        if (currentPresetIndex >= numInCategory) currentPresetIndex = 0;

        audioProcessor.loadReverbPreset (currentCategory, currentPresetIndex);
        audioProcessor.savedPresetIndex[0] = categoryToFlatIndex (currentCategory, currentPresetIndex);
        updatePresetName();
    }

    void updatePresetName()
    {
        if (currentCategory >= 0 && currentCategory < numCategories
            && currentPresetIndex >= 0 && currentPresetIndex < presetsPerCategory[currentCategory])
        {
            presetNameLabel.setText (categoryPresetNames[currentCategory][currentPresetIndex],
                                    juce::dontSendNotification);
        }
    }

    void showPresetMenu()
    {
        juce::PopupMenu menu;
        int itemId = 1;

        for (int c = 0; c < numCategories; ++c)
        {
            juce::PopupMenu subMenu;
            for (int s = 0; s < presetsPerCategory[c]; ++s)
            {
                bool isCurrent = (c == currentCategory && s == currentPresetIndex);
                subMenu.addItem (itemId, categoryPresetNames[c][s], true, isCurrent);
                ++itemId;
            }
            menu.addSubMenu (categoryNames[c], subMenu, true, juce::Image(), currentCategory == c);
        }

        menu.showMenuAsync (juce::PopupMenu::Options(),
            [this] (int result) {
                if (result > 0) {
                    int flatIdx = result - 1;
                    flatIndexToCategory (flatIdx, currentCategory, currentPresetIndex);
                    audioProcessor.loadReverbPreset (currentCategory, currentPresetIndex);
                    audioProcessor.savedPresetIndex[0] = flatIdx;
                    updatePresetName();
                }
            });
    }

    void layoutKnob (juce::Label& label, juce::Slider& knob, int x, int y, int w, int lh, int kh)
    {
        label.setBounds (x, y, w, lh);
        knob.setBounds (x, y + lh, w, kh);
    }

    void layoutFader (juce::Label& label, juce::Slider& fader, int x, int y, int w, int lh, int fh)
    {
        label.setBounds (x, y, w, lh);
        fader.setBounds (x, y + lh, w, fh);
    }

    // =========================================================================
    // Shared grid helpers
    // =========================================================================
    juce::Rectangle<int> getGridInner (juce::Rectangle<int> area) const
    {
        return area.reduced (gridPad, 0).withTrimmedTop (gridTopInset).withTrimmedBottom (gridBotInset);
    }

    float freqToNormX (float freq) const
    {
        return (std::log10 (freq) - logMin) / (logMax - logMin);
    }

    juce::Rectangle<int> getGridArea() const
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop (topStripH);
        int gridH = juce::roundToInt (bounds.getHeight() * gridProportion);
        return bounds.removeFromTop (gridH);
    }

    juce::Rectangle<int> getOutputEQInner() const
    {
        auto g = getGridArea();
        return g.reduced (gridPad, 0)
                .withTrimmedTop (gridTopInset + eqBtnStripH)
                .withTrimmedBottom (gridBotInset);
    }

    juce::Rectangle<int> getDampingEQInner() const
    {
        return getGridInner (getGridArea());
    }

    std::array<juce::Point<float>, 5> computeDampingDotPositions (juce::Rectangle<int> inner) const
    {
        const float defaultFreqs[5] = { 60.0f, 250.0f, 1000.0f, 4000.0f, 12000.0f };
        const char* paramIds[5] = { "attack", "size", "density", "decay", "distance" };

        std::array<juce::Point<float>, 5> result;

        for (int i = 0; i < 5; ++i)
        {
            // --- X: read from dampDotFreq parameter (set by drag) ---
            float freq = defaultFreqs[i];
            juce::String freqParamId = "dampDotFreq" + juce::String (i + 1);
            if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    apvts.getParameter (freqParamId)))
                freq = p->get();

            float nx = freqToNormX (freq);
            float px = inner.getX() + nx * inner.getWidth();

            // --- Y: read from the parameter this dot controls (0-100 range) ---
            float paramVal = 50.0f;
            if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    apvts.getParameter (paramIds[i])))
                paramVal = p->get();

            // Scale down to match the 0.25f scaling in drawDampingEQ
            float normVal = juce::jlimit (0.0f, 1.0f, paramVal / 100.0f) * 0.25f;
            float py = inner.getY() + (1.0f - normVal) * inner.getHeight();

            result[(size_t) i] = { px, py };
        }

        return result;
    }

    static const char* dampingDotParamId (int dotIndex)
    {
        // Each dot now controls its own unique parameter
        // Left to right: Attack, Size, Density, Decay, Distance
        switch (dotIndex)
        {
            case 1: return "attack";
            case 2: return "size";
            case 3: return "density";
            case 4: return "decay";
            case 5: return "distance";
            default: return nullptr;
        }
    }

    void drawAuroraGlow (juce::Graphics& g, juce::Rectangle<int> inner,
                         float hue, float brightness)
    {
        if (brightness < 0.005f)
            return;

        const float hueOffsets[4]  = { 0.0f, 0.08f, 0.16f, 0.24f };
        const float xFractions[4]  = { 0.15f, 0.38f, 0.62f, 0.85f };
        const float yFraction      = 0.45f;

        for (int i = 0; i < 4; ++i)
        {
            float glowHue = hue + hueOffsets[i];
            if (glowHue > 1.0f) glowHue -= 1.0f;

            auto glowColour = juce::Colour::fromHSV (glowHue, 0.85f, 1.0f, 1.0f);

            float cx = inner.getX() + xFractions[i] * inner.getWidth();
            float cy = inner.getY() + yFraction * inner.getHeight();
            float radius = inner.getWidth() * 0.35f;

            juce::ColourGradient grad (
                glowColour.withAlpha (brightness * 0.30f),
                cx, cy,
                glowColour.withAlpha (0.0f),
                cx + radius, cy,
                true
            );

            g.setGradientFill (grad);
            g.fillEllipse (cx - radius, cy - radius * 0.7f,
                           radius * 2.0f, radius * 1.4f);
        }
    }

    void drawFreqVerticalLines (juce::Graphics& g, juce::Rectangle<int> inner)
    {
        const float freqs[]      = { 20, 30, 40, 60, 80, 100, 200, 300, 500, 800,
                                     1000, 2000, 3000, 5000, 8000, 10000, 20000 };
        const char* freqLabels[] = { "20", "30", "40", "60", "80", "100", "200", "300", "500", "800",
                                     "1k", "2k", "3k", "5k", "8k", "10k", "20k" };
        int numFreqs = 17;

        g.setFont (juce::FontOptions (10.0f));

        for (int i = 0; i < numFreqs; ++i)
        {
            float norm = freqToNormX (freqs[i]);
            float x = inner.getX() + norm * inner.getWidth();

            bool isMajor = (freqs[i] == 100 || freqs[i] == 1000 || freqs[i] == 10000);
            g.setColour (isMajor ? gridLineMajor : gridLine);
            g.drawVerticalLine (juce::roundToInt (x), (float) inner.getY(),
                                (float) inner.getBottom());

            g.setColour (gridLabelColour);
            g.drawText (freqLabels[i],
                        juce::roundToInt (x) - 15, inner.getBottom() + 2, 30, 14,
                        juce::Justification::centred, false);
        }
    }

    // =========================================================================
    // MAIN page: Damping EQ
    // =========================================================================
    void drawDampingEQ (juce::Graphics& g, juce::Rectangle<int> area)
    {
        auto inner = getGridInner (area);

        // --- Title ---
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("DAMPING EQ", inner.getX(), area.getY() + 10, 120, 16,
                    juce::Justification::centredLeft, false);

        // --- Grid ---
        int numHLines = 5;
        for (int i = 0; i <= numHLines; ++i)
        {
            float y = inner.getY() + (inner.getHeight() * i / (float) numHLines);
            g.setColour ((i == 0 || i == numHLines) ? gridLineMajor : gridLine);
            g.drawHorizontalLine (juce::roundToInt (y), (float) inner.getX(),
                                  (float) inner.getRight());
        }
        drawFreqVerticalLines (g, inner);

        // --- % labels ---
        g.setFont (juce::FontOptions (10.0f));
        const char* leftLabels[] = { "100%", "80%", "60%", "40%", "20%", "0%" };
        for (int i = 0; i <= 5; ++i)
        {
            float y = inner.getY() + (inner.getHeight() * i / 5.0f);
            g.setColour (gridLabelColour);
            g.drawText (leftLabels[i], inner.getX() - gridPad,
                        juce::roundToInt (y) - 7, gridPad - 4, 14,
                        juce::Justification::centredRight, false);
        }

        // --- Read parameters ---
        const char* paramIds[5]     = { "attack", "size", "density", "decay", "distance" };
        const float defaultFreqs[5] = { 60.0f, 250.0f, 1000.0f, 4000.0f, 12000.0f };
        // Bell widths in log-freq units (larger = wider hill)
        // Attack and Distance are wider to behave like shelves
        const float bellWidths[5]   = { 1.2f, 0.8f, 0.7f, 0.8f, 1.2f };

        float paramVals[5];
        float dotFreqs[5];

        for (int i = 0; i < 5; ++i)
        {
            paramVals[i] = 50.0f;
            if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    apvts.getParameter (paramIds[i])))
                paramVals[i] = p->get();

            dotFreqs[i] = defaultFreqs[i];
            juce::String freqId = "dampDotFreq" + juce::String (i + 1);
            if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    apvts.getParameter (freqId)))
                dotFreqs[i] = p->get();
        }

        // --- Compute gaussian hill for each band across numPoints ---
        constexpr int numPoints = 256;
        float logMin = std::log10 (20.0f);
        float logMax = std::log10 (20000.0f);

        // bandCurves[band][point] = height 0-1
        std::array<std::array<float, numPoints>, 5> bandCurves;

        for (int b = 0; b < 5; ++b)
        {
            // Scale down each band so the composite sum of all 5 stays within the graph
            float height    = juce::jlimit (0.0f, 1.0f, paramVals[b] / 100.0f) * 0.25f;
            float centreLog = std::log10 (juce::jmax (dotFreqs[b], 20.0f));
            float sigma     = bellWidths[b];

            for (int k = 0; k < numPoints; ++k)
            {
                float t    = k / (float) (numPoints - 1);
                float logF = logMin + t * (logMax - logMin);
                float diff = (logF - centreLog) / sigma;
                bandCurves[b][k] = height * std::exp (-0.5f * diff * diff);
            }
        }

        // --- Compute composite sum curve (clamped to 0-1) ---
        std::array<float, numPoints> sumCurve;
        for (int k = 0; k < numPoints; ++k)
        {
            float s = 0.0f;
            for (int b = 0; b < 5; ++b)
                s += bandCurves[b][k];
            sumCurve[k] = juce::jlimit (0.0f, 1.0f, s);
        }

        // --- Helper: build a juce::Path from a float array ---
        auto buildPath = [&] (const float* curve, int n) -> juce::Path
        {
            juce::Path path;
            for (int k = 0; k < n; ++k)
            {
                float px = inner.getX() + (k / (float) (n - 1)) * inner.getWidth();
                float py = inner.getY() + (1.0f - curve[k]) * inner.getHeight();
                if (k == 0) path.startNewSubPath (px, py);
                else        path.lineTo (px, py);
            }
            return path;
        };

        // --- Aurora glow (behind everything) ---
        drawAuroraGlow (g, inner, auroraHue, auroraBrightness);

        // --- Draw decay snapshot curves behind everything ---
        float decayTime = 1.1f;
        if (auto* dp = dynamic_cast<juce::AudioParameterFloat*> (
                apvts.getParameter ("decay")))
            decayTime = dp->get();

        static constexpr int numDecayCurves = 5;
        float decayBase = juce::jmap (decayTime, 0.3f, 10.0f, 1.2f, 1.6f);
        const float powers[numDecayCurves] = {
            decayBase, decayBase * 1.8f, decayBase * 3.2f,
            decayBase * 5.5f, decayBase * 9.0f
        };

        for (int c = numDecayCurves - 1; c >= 0; --c)
        {
            std::array<float, numPoints> decayCurve;
            for (int k = 0; k < numPoints; ++k)
                decayCurve[k] = std::pow (sumCurve[k], powers[c]);

            float alpha = juce::jmap ((float) c, 0.0f, (float) (numDecayCurves - 1),
                                      0.45f, 0.10f);
            g.setColour (knobArc.withAlpha (alpha));
            g.strokePath (buildPath (decayCurve.data(), numPoints),
                          juce::PathStrokeType (1.2f));
        }

        // --- Draw individual band curves (dim) ---
        for (int b = 0; b < 5; ++b)
        {
            g.setColour (knobArc.withAlpha (0.18f));
            g.strokePath (buildPath (bandCurves[b].data(), numPoints),
                          juce::PathStrokeType (1.0f));
        }

        // --- Draw composite sum curve (bold, filled) ---
        {
            juce::Path sumPath = buildPath (sumCurve.data(), numPoints);

            juce::Path fillPath (sumPath);
            float zeroY = inner.getY() + inner.getHeight();
            fillPath.lineTo ((float) inner.getRight(), zeroY);
            fillPath.lineTo ((float) inner.getX(), zeroY);
            fillPath.closeSubPath();

            g.setColour (knobArc.withAlpha (0.10f));
            g.fillPath (fillPath);

            if (auroraBrightness > 0.05f)
            {
                auto auroraColour = juce::Colour::fromHSV (auroraHue, 0.9f, 1.0f, 1.0f);
                auto curveColour  = knobArc.interpolatedWith (auroraColour, auroraBrightness * 0.7f);
                g.setColour (curveColour.withAlpha (0.90f));
            }
            else
            {
                g.setColour (knobArc.withAlpha (0.90f));
            }
            g.strokePath (sumPath, juce::PathStrokeType (2.0f));
        }

        // --- Draw control point dots ---
        auto dots = computeDampingDotPositions (inner);
        for (int i = 0; i < 5; ++i)
        {
            bool isDragged = (draggedDampingDot == i);
            float r = isDragged ? 6.0f : 4.0f;
            auto pt = dots[(size_t) i];

            g.setColour (knobArc);
            g.fillEllipse (pt.x - r, pt.y - r, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.drawEllipse (pt.x - r, pt.y - r, r * 2.0f, r * 2.0f, 1.0f);
        }

        // --- Right-side time labels ---
        g.setFont (juce::FontOptions (10.0f));
        int numTimeLabels = 5;
        for (int t = 1; t <= numTimeLabels; ++t)
        {
            float yFrac  = (float) t / (float) numTimeLabels;
            float y      = inner.getY() + yFrac * inner.getHeight();
            float timeVal = decayTime * yFrac;
            juce::String timeStr = (timeVal < 10.0f)
                ? juce::String (timeVal, 1) + "s"
                : juce::String (juce::roundToInt (timeVal)) + "s";
            g.setColour (gridLabelColour);
            g.drawText (timeStr, inner.getRight() + 4,
                        juce::roundToInt (y) - 7, gridPad - 4, 14,
                        juce::Justification::centredLeft, false);
        }
    }

    // =========================================================================
    // DETAILS page: Output EQ
    // =========================================================================
    void drawOutputEQ (juce::Graphics& g, juce::Rectangle<int> area)
    {
        auto inner = area.reduced (gridPad, 0)
            .withTrimmedTop (gridTopInset + eqBtnStripH)
            .withTrimmedBottom (gridBotInset);

        // --- Title ---
        bool eqMasterOn = outputEqButton.getToggleState();
        g.setColour (eqMasterOn ? knobArc : knobArc.withAlpha (0.35f));
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("OUTPUT EQ", inner.getX() + 22, area.getY() + 10, 120, 16,
                    juce::Justification::centredLeft, false);

        // --- Horizontal grid lines (dB scale: +24 to -24) ---
        int numHLines = 8;
        for (int i = 0; i <= numHLines; ++i)
        {
            float y = inner.getY() + (inner.getHeight() * i / (float) numHLines);
            bool isZero = (i == numHLines / 2);
            bool isEdge = (i == 0 || i == numHLines);

            if (isZero)
                g.setColour (juce::Colours::white.withAlpha (0.22f));
            else if (isEdge)
                g.setColour (gridLineMajor);
            else
                g.setColour (gridLine);

            g.drawHorizontalLine (juce::roundToInt (y), (float) inner.getX(),
                                  (float) inner.getRight());
        }

        drawFreqVerticalLines (g, inner);

        // --- dB labels ---
        g.setFont (juce::FontOptions (10.0f));
        const char* dbLabels[] = { "+24", "+18", "+12", "+6", "0", "-6", "-12", "-18", "-24" };
        for (int i = 0; i <= 8; ++i)
        {
            float y = inner.getY() + (inner.getHeight() * i / 8.0f);
            bool isZero = (i == 4);
            g.setColour (isZero ? juce::Colours::white.withAlpha (0.55f) : gridLabelColour);
            g.drawText (dbLabels[i],
                        inner.getX() - gridPad, juce::roundToInt (y) - 7, gridPad - 4, 14,
                        juce::Justification::centredRight, false);
        }

        float zeroY = inner.getY() + inner.getHeight() * 0.5f;

        // --- Compute dot positions from gain parameters ---
        float dotX[8], dotY[8];
        for (int i = 0; i < 8; ++i)
        {
            float gainDb = 0.0f;
            juce::String paramId = "eqBand" + juce::String (i + 1) + "Gain";
            if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (paramId)))
                gainDb = p->get();

            float nx = freqToNormX (eqBandFreqs[i]);
            dotX[i] = inner.getX() + nx * inner.getWidth();
            dotY[i] = inner.getY() + (24.0f - gainDb) / 48.0f * inner.getHeight();
        }

        // --- Draw smooth response curve through EQ dots ---
        {
            juce::Path curvePath;
            curvePath.startNewSubPath ((float) inner.getX(), dotY[0]);
            curvePath.lineTo (dotX[0], dotY[0]);

            for (int i = 0; i < 7; ++i)
            {
                float cpOff = (dotX[i + 1] - dotX[i]) * 0.4f;
                curvePath.cubicTo (dotX[i] + cpOff, dotY[i],
                                   dotX[i + 1] - cpOff, dotY[i + 1],
                                   dotX[i + 1], dotY[i + 1]);
            }

            curvePath.lineTo ((float) inner.getRight(), dotY[7]);

            // Fill between curve and 0dB line
            juce::Path fillPath (curvePath);
            fillPath.lineTo ((float) inner.getRight(), zeroY);
            fillPath.lineTo ((float) inner.getX(), zeroY);
            fillPath.closeSubPath();

            g.setColour (knobArc.withAlpha (eqMasterOn ? 0.10f : 0.03f));
            g.fillPath (fillPath);

            g.setColour (knobArc.withAlpha (eqMasterOn ? 0.8f : 0.2f));
            g.strokePath (curvePath, juce::PathStrokeType (2.0f));
        }

        // --- Control point dots ---
        for (int i = 0; i < 8; ++i)
        {
            bool bandOn = eqMasterOn && eqBandButtons[i].getToggleState();
            bool isDragged = (draggedOutputEQBand == i);
            float r = isDragged ? 7.0f : 5.0f;

            g.setColour (bandOn ? knobArc : knobArc.withAlpha (0.15f));
            g.fillEllipse (dotX[i] - r, dotY[i] - r, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.drawEllipse (dotX[i] - r, dotY[i] - r, r * 2.0f, r * 2.0f, 1.0f);
        }
    }

    // =========================================================================
    // Members
    // =========================================================================
    APOLLOAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;

    // Preset management
    int currentPresetIndex = 0;
    int currentCategory = 0;

    static constexpr int numCategories = 7;
    static constexpr int totalReverbPresets = 40; // 6+6+5+6+6+5+6

    const juce::String categoryNames[7] = {
        "Small Rooms", "Halls", "Theaters", "Plates & Digital",
        "Creative", "Vocals", "Ambient/FX"
    };

    const int presetsPerCategory[7] = { 6, 6, 5, 6, 6, 5, 6 };

    const juce::String categoryPresetNames[7][6] = {
        { "Tight Drum Room", "Warm Vocal Booth", "Bright Chamber", "Dark Closet", "Live Room", "Snare Room" },
        { "Concert Grand", "Vocal Hall", "Orchestral", "Bright Hall", "Warm Hall", "Recital Hall" },
        { "Classic Theater", "Cinematic", "Film Score", "Dialogue Stage", "Drama Hall", "" },
        { "Bright Plate", "Warm Plate", "Vocal Plate", "Drum Plate", "Thick Plate", "Shimmer" },
        { "Synth Wash", "Ethereal", "Strange Space", "Blooming Pad", "Glitch Verb", "Airy Texture" },
        { "Intimate Vocal", "Pop Vocal", "Ballad Vocal", "Radio Vocal", "Choir", "" },
        { "Infinite Space", "Frozen Air", "Dark Atmosphere", "Cosmic Delay", "Drone Pad", "Ocean Cave" },
    };

    int categoryToFlatIndex (int cat, int sub) const
    {
        int flat = 0;
        for (int c = 0; c < cat; ++c)
            flat += presetsPerCategory[c];
        return flat + sub;
    }

    void flatIndexToCategory (int flat, int& cat, int& sub) const
    {
        cat = 0;
        int remaining = flat;
        while (cat < numCategories - 1 && remaining >= presetsPerCategory[cat])
        {
            remaining -= presetsPerCategory[cat];
            ++cat;
        }
        sub = juce::jlimit (0, presetsPerCategory[cat] - 1, remaining);
    }

    // Output EQ graph component
    std::unique_ptr<OutputEQGraph> eqGraph;

    int currentSubPage = 0;

    juce::TextButton mainButton { "MAIN" };
    juce::TextButton detailsButton { "DETAILS" };

    // MAIN knobs
    juce::Slider attackKnob, sizeKnob, densityKnob, decayKnob, distanceKnob, predelayKnob;
    juce::Label  attackLabel, sizeLabel, densityLabel, decayLabel, distanceLabel, predelayLabel;

    // MAIN faders
    juce::Slider drySlider, wetSlider;
    juce::Label  dryLabel, wetLabel;

    // MAIN freeze
    juce::ToggleButton freezeButton;
    juce::Label decayMinLabel, decayMaxLabel;

    // DETAILS knobs
    juce::Slider qualityKnob, modSpeedKnob, modDepthKnob, smoothingKnob,
                 earlyLateKnob, widthKnob, monoMakerKnob;
    juce::Label  qualityLabel, modSpeedLabel, modDepthLabel, smoothingLabel,
                 earlyLateLabel, widthLabel, monoMakerLabel;

    // MAIN attachments
    std::unique_ptr<SliderAttachment> attackAttach, sizeAttach, densityAttach,
                                      decayAttach, distanceAttach, predelayAttach,
                                      dryAttach, wetAttach;
    std::unique_ptr<ButtonAttachment> freezeAttach;

    // DETAILS attachments
    std::unique_ptr<SliderAttachment> qualityAttach, modSpeedAttach, modDepthAttach,
                                      smoothingAttach, earlyLateAttach, widthAttach,
                                      monoMakerAttach;

    // Mod Source waveform buttons (DETAILS page)
    WaveformButton modSourceBtns[3];
    juce::Label    modSourceLabel;

    // Mono Maker power button (DETAILS page)
    PowerButton monoMakerPowerBtn;
    std::unique_ptr<ButtonAttachment> monoMakerPowerAttach;

    // Output EQ master power button
    PowerButton outputEqButton;
    std::unique_ptr<ButtonAttachment> outputEqAttach;

    // Master reverb power button (top strip)
    PowerButton masterPowerBtn;
    std::unique_ptr<ButtonAttachment> masterPowerAttach;

    // Room type selector (grid area, MAIN only)
    juce::ComboBox roomSelector;
    std::unique_ptr<ComboBoxAttachment> roomAttach;

    // Preset strip
    juce::TextButton presetCategoryButton;
    juce::Label      presetNameLabel;
    juce::TextButton prevPresetButton;
    juce::TextButton nextPresetButton;
    juce::TextButton savePresetButton;

    // Time / Note mode buttons (MAIN page)
    TimeNoteButton decayModeBtns[2];     // [0]=Time, [1]=Note
    TimeNoteButton predelayModeBtns[2];  // [0]=Time, [1]=Note

    // EQ band shape buttons
    EQBandButton eqBandButtons[8];
    std::unique_ptr<ButtonAttachment> eqBandAttachments[8];

    // Interactive EQ drag state
    int draggedOutputEQBand = -1;   // -1 = none, 0-7 = which band
    int   draggedDampingDot  = -1;   // -1 = none, 1-5 = which dot
    float dampDragStartX     = 0.0f; // mouse X at drag start
    float dampDragStartY     = 0.0f; // mouse Y at drag start
    float dampDragStartFreq  = 0.0f; // parameter freq value at drag start
    float dampDragStartParam = 0.0f; // Y parameter value at drag start (0-1 normalised)

    // Aurora glow state
    std::atomic<float>& wetLevelRef;     // reference to processor's level meter
    float auroraHue        = 0.0f;       // 0-1, cycles continuously
    float auroraBrightness = 0.0f;       // 0-1, driven by wet level

    // Layout constants
    static constexpr int   topStripH      = 36;
    static constexpr int   powerBtnSize   = 24;
    static constexpr int   roomBtnW       = 170;
    static constexpr int   roomBtnH       = 26;
    static constexpr int   roomBtnGap     = 8;
    static constexpr float gridProportion = 0.6f;
    static constexpr int   gridPad        = 40;
    static constexpr int   gridTopInset   = 40;
    static constexpr int   gridBotInset   = 20;
    static constexpr int   eqBtnStripH   = 26;
    static constexpr float eqBandFreqs[8] = { 40.0f, 100.0f, 250.0f, 800.0f, 2500.0f, 6000.0f, 10000.0f, 16000.0f };

    // Frequency mapping
    static constexpr float logMin = 1.30103f;   // log10(20)
    static constexpr float logMax = 4.30103f;   // log10(20000)

    // Colors
    const juce::Colour topStripBg      = juce::Colour::fromRGB (35, 37, 42);
    const juce::Colour gridBackground  = juce::Colour::fromRGB (28, 30, 38);
    const juce::Colour gridLine        = juce::Colours::white.withAlpha (0.07f);
    const juce::Colour gridLineMajor   = juce::Colours::white.withAlpha (0.12f);
    const juce::Colour gridLabelColour = juce::Colours::white.withAlpha (0.35f);
    const juce::Colour silverTop       = juce::Colour::fromRGB (132, 136, 142);
    const juce::Colour silverBottom    = juce::Colour::fromRGB (102, 106, 112);
    const juce::Colour knobArc         = juce::Colour::fromRGB (230, 176, 46);
    const juce::Colour knobTrack       = juce::Colour::fromRGB (55, 57, 62);
    const juce::Colour subTabInactive  = juce::Colour::fromRGB (45, 47, 52);
    const juce::Colour subTabActive    = juce::Colour::fromRGB (60, 62, 68);
    const juce::Colour presetBtnColour = juce::Colour::fromRGB (48, 50, 56);
    const juce::Colour presetNameBg    = juce::Colour::fromRGB (38, 40, 46);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbPage)
};
