/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// EQ Band data
struct EQBand
{
    float frequency  = 1000.0f;   // Hz
    float gainDB     = 0.0f;      // dB boost/cut
    float Q          = 1.0f;      // quality factor
    int   filterType = 0;         // 0=bell, 1=lowCut(HP), 2=highCut(LP), 3=lowShelf, 4=highShelf, 5=notch
    juce::Colour colour;
};

//==============================================================================
// Custom LookAndFeel for the dark top bar controls
class TopBarLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TopBarLookAndFeel()
    {
        setColour(juce::ComboBox::backgroundColourId,   juce::Colour(0xFF1A1A2E));
        setColour(juce::ComboBox::outlineColourId,      juce::Colour(0xFF0DCDD4).withAlpha(0.4f));
        setColour(juce::ComboBox::textColourId,         juce::Colours::white.withAlpha(0.9f));
        setColour(juce::ComboBox::arrowColourId,        juce::Colour(0xFF0DCDD4));

        setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(0xFF111122));
        setColour(juce::PopupMenu::textColourId,                  juce::Colours::white.withAlpha(0.9f));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xFF0DCDD4).withAlpha(0.3f));
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        juce::Path arrow;
        float arrowX = (float)width - 18.0f;
        float arrowY = (float)height * 0.5f - 2.0f;
        arrow.addTriangle(arrowX, arrowY, arrowX + 8.0f, arrowY, arrowX + 4.0f, arrowY + 5.0f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId));
        g.fillPath(arrow);
    }
};

//==============================================================================
// Elegant premium knob LookAndFeel
// Custom LookAndFeel for Power Button
class PowerButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    bool bypassed = false;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                               const juce::Colour&, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(juce::Colour(0xFF1A1A2E));
        g.fillRoundedRectangle(bounds, 4.0f);

        juce::Colour borderCol = bypassed
            ? juce::Colour(0xFF555555).withAlpha(0.4f)
            : juce::Colour(0xFF0DCDD4).withAlpha(isMouseOver ? 0.6f : 0.4f);
        g.setColour(borderCol);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                         bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float r = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.28f;

        juce::Colour iconCol = bypassed
            ? juce::Colour(0xFF555555)
            : juce::Colour(0xFF0DCDD4);

        // Power arc (open at top)
        juce::Path arc;
        arc.addCentredArc(cx, cy + 1.0f, r, r, 0.0f,
                          juce::MathConstants<float>::pi * 0.3f,
                          juce::MathConstants<float>::pi * 1.7f, true);
        g.setColour(iconCol);
        g.strokePath(arc, juce::PathStrokeType(2.0f));

        // Vertical line at top
        g.drawLine(cx, cy - r, cx, cy + 1.0f, 2.0f);
    }
};

class ElegantKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ElegantKnobLookAndFeel() {}

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6.0f);
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float centreX = bounds.getCentreX();
        float centreY = bounds.getCentreY();
        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // === Outer shadow ring ===
        g.setColour(juce::Colour(0xFF000000).withAlpha(0.4f));
        g.fillEllipse(centreX - radius - 2, centreY - radius - 2,
                      (radius + 2) * 2, (radius + 2) * 2);

        // === Outer ring (dark metallic border) ===
        {
            juce::ColourGradient ringGrad(
                juce::Colour(0xFF3A3A50), centreX, centreY - radius,
                juce::Colour(0xFF1A1A28), centreX, centreY + radius, false);
            g.setGradientFill(ringGrad);
            g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);
        }

        // === Inner knob face (brushed metal gradient) ===
        float innerRadius = radius * 0.82f;
        {
            juce::ColourGradient faceGrad(
                juce::Colour(0xFF484860), centreX, centreY - innerRadius,
                juce::Colour(0xFF1E1E30), centreX, centreY + innerRadius, false);
            g.setGradientFill(faceGrad);
            g.fillEllipse(centreX - innerRadius, centreY - innerRadius,
                          innerRadius * 2, innerRadius * 2);
        }

        // === Subtle top highlight (specular) ===
        {
            float highlightR = innerRadius * 0.7f;
            juce::ColourGradient specGrad(
                juce::Colours::white.withAlpha(0.12f), centreX, centreY - innerRadius * 0.5f,
                juce::Colours::transparentWhite, centreX, centreY, true);
            g.setGradientFill(specGrad);
            g.fillEllipse(centreX - highlightR, centreY - innerRadius * 0.6f,
                          highlightR * 2, highlightR * 1.2f);
        }

        // === Arc track (background) ===
        float arcRadius = radius * 0.92f;
        float arcThickness = 3.0f;
        {
            juce::Path bgArc;
            bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xFF1A1A28));
            g.strokePath(bgArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        // === Arc track (filled / active) ===
        {
            juce::Path activeArc;
            activeArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, angle, true);
            g.setColour(juce::Colour(0xFF0DCDD4));
            g.strokePath(activeArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

            // Glow on arc
            g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.15f));
            g.strokePath(activeArc, juce::PathStrokeType(arcThickness + 4.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }

        // === Pointer / indicator line ===
        {
            float pointerLength = innerRadius * 0.55f;
            float pointerThickness = 2.5f;
            float startDist = innerRadius * 0.2f;

            juce::Path pointer;
            pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength,
                                         pointerThickness, pointerLength - startDist, 1.0f);
            pointer.applyTransform(juce::AffineTransform::rotation(angle)
                                       .translated(centreX, centreY));

            // Pointer glow
            g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.3f));
            g.strokePath(pointer, juce::PathStrokeType(4.0f));

            // Pointer line
            g.setColour(juce::Colours::white.withAlpha(0.95f));
            g.fillPath(pointer);
        }

        // === Center dot ===
        float dotR = 3.0f;
        g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.6f));
        g.fillEllipse(centreX - dotR, centreY - dotR, dotR * 2, dotR * 2);
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
        label->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
        label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        label->setFont(juce::FontOptions(11.0f));
        label->setJustificationType(juce::Justification::centred);
        return label;
    }
};

//==============================================================================
class OrionSoundEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    OrionSoundEQAudioProcessorEditor (OrionSoundEQAudioProcessor&);
    ~OrionSoundEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void timerCallback() override;

private:
    // Drawing helpers
    void drawEQGrid(juce::Graphics& g);
    void drawBandCurves(juce::Graphics& g, juce::Rectangle<float> panel);
    void drawBandHandles(juce::Graphics& g, juce::Rectangle<float> panel);
    void drawOutputMeter(juce::Graphics& g);
    void drawKnobPanel(juce::Graphics& g);
    void drawSettingsPage(juce::Graphics& g);
    void setPageVisibility(bool mainPage);

    // EQ engine helpers
    void updateBands(int bandCount);
    float getBandGainAtFreq(const EQBand& band, float freq) const;
    static juce::Colour getColourForBand(int index, int total);

    // Spectrum analyzer overlay
    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> panel);
    float spectrumData[OrionSoundEQAudioProcessor::numBins] = {};

    // Coordinate mapping
    float freqToX(float freq, float panelX, float panelW) const;
    float dBToY(float dB, float panelY, float panelH) const;
    float xToFreq(float x, float panelX, float panelW) const;
    float yToDB(float y, float panelY, float panelH) const;
    juce::Rectangle<float> getPanelRect() const;

    // Hit-testing
    int getHandleAtPosition(juce::Point<float> pos) const;

    // Top bar height
    static constexpr float topBarHeight = 36.0f;

    // dB range
    static constexpr float dBMin = -24.0f;
    static constexpr float dBMax =  24.0f;

    // Custom LookAndFeel
    TopBarLookAndFeel topBarLnf;
    PowerButtonLookAndFeel powerLnf;
    ElegantKnobLookAndFeel knobLnf;

    // Top bar controls
    juce::TextButton mainPageButton   { "Main" };
    juce::TextButton settingsButton   { "Settings" };
    juce::ComboBox   bandCountCombo;
    juce::ComboBox   bandModeCombo;
    juce::ComboBox   filterCombo;
    juce::ComboBox   presetCombo;
    juce::TextButton savePresetButton { "Save" };

    // Preset data structure
    struct PresetPoint { float freq; float gain; float Q; int filterType; };
    void applyPresetPoints(const std::vector<PresetPoint>& pts);
    void applyPresetById(int presetId);
    void rebuildPresetMenu();      // rebuilds the preset combo menu including user presets
    void saveUserPreset();         // prompts for name and saves current EQ as user preset
    void applyUserPreset(int userPresetIndex);  // loads a user preset by index
    juce::TextButton powerButton      { "" };
    bool eqBypassed = false;
    juce::TextButton undoButton       { "Undo" };
    juce::TextButton redoButton       { "Redo" };

    // Bottom bar controls
    juce::ComboBox   phaseModeCombo;
    juce::TextButton spectrumButton   { "Spectrum" };
    juce::TextButton midiLearnButton  { "MIDI Learn" };

    // Knob panel controls
    juce::Slider freqKnob;
    juce::Slider gainKnob;
    juce::Slider qKnob;
    juce::Label  freqKnobLabel;
    juce::Label  gainKnobLabel;
    juce::Label  qKnobLabel;

    // EQ bands
    std::vector<EQBand> bands;

    // Drag / selection state
    int  dragBandIndex    = -1;
    int  hoveredBandIndex = -1;
    int  selectedBandIndex = -1;  // currently selected band (clicked)

    // Output gain slider (vertical bar on right)
    float outputGainDB      = 0.0f;   // current output gain in dB (-24 to +24)
    bool  draggingOutputGain = false;

    // Page state
    bool showingMainPage = true;  // true = EQ page, false = Settings page

    // Customizable key binds
    struct CustomKeyBinds
    {
        int deletePointKey      = juce::KeyPress::backspaceKey;
        int undoKey             = 'z';  // Cmd/Ctrl+Z
        int redoKey             = 'z';  // Cmd/Ctrl+Shift+Z
        int undoKeyMods         = juce::ModifierKeys::commandModifier;
        int redoKeyMods         = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;
    };
    CustomKeyBinds customKeyBinds;
    int editingKeybindIndex = -1;  // -1 = not editing, 0-3 = which keybind is being edited

    // Undo / Redo history
    struct EQSnapshot
    {
        std::vector<EQBand> bands;
        int selectedBandIndex;
        int bandModeId;
    };
    std::vector<EQSnapshot> undoStack;
    std::vector<EQSnapshot> redoStack;
    static constexpr int maxUndoHistory = 100;
    void pushUndoState();
    void performUndo();
    void performRedo();
    void addBandAtPosition(float freq, float gainDB);
    void deleteSelectedBand();

    // Aurora animation
    float animPhase = 0.0f;
    float auroraLevel = 0.0f;  // smoothed 0..1, driven by audio RMS

    OrionSoundEQAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrionSoundEQAudioProcessorEditor)
};
