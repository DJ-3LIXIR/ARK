/*
  ==============================================================================
    HADES - Plugin Editor
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================
//  Constructor
// ============================================================
HadesAudioProcessorEditor::HadesAudioProcessorEditor (HadesAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      guitarPage  (p),
      bassPage    (p),
      cabinetPage (p),
      settingsPage (p)
{
    setSize (1100, 620);

    // Add pages
    addChildComponent (guitarPage);
    addChildComponent (bassPage);
    addChildComponent (cabinetPage);
    addChildComponent (settingsPage);

    // Setup nav buttons
    auto setupBtn = [this](juce::TextButton& btn, int pageIndex)
    {
        btn.setLookAndFeel (&hadesLAF);
        btn.setClickingTogglesState (false);
        btn.setRadioGroupId (1);
        btn.onClick = [this, pageIndex] { showPage (pageIndex); };
        addAndMakeVisible (btn);
    };

    setupBtn (btnGuitar,   0);
    setupBtn (btnBass,     1);
    setupBtn (btnCabinet,  2);
    setupBtn (btnSettings, 3);

    // Setup preset dropdown
    presetBox.setLookAndFeel (&hadesLAF);
    presetBox.setColour (juce::ComboBox::backgroundColourId,  juce::Colour (0xFF1A0505));
    presetBox.setColour (juce::ComboBox::outlineColourId,     HadesColours::crimson.withAlpha (0.5f));
    presetBox.setColour (juce::ComboBox::textColourId,        HadesColours::offWhite);
    presetBox.setColour (juce::ComboBox::arrowColourId,       HadesColours::ember);
    presetBox.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (presetBox);

    // Wire preset selection to processor preset loaders
    presetBox.onChange = [this]
    {
        int sel = presetBox.getSelectedId() - 1; // convert to 0-based index
        if (sel < 0) return;

        // Remember this selection for the current tab
        selectedPresetPerPage[currentPage] = sel;
        audioProcessor.savedPresetIndex[currentPage] = sel;

        if (currentPage == 0)
            audioProcessor.loadGuitarPreset (sel);
        else if (currentPage == 1)
            audioProcessor.loadBassPreset (sel);
        else if (currentPage == 2)
            audioProcessor.loadCabinetPreset (sel);
    };

    btnGuitar.setToggleState (true, juce::dontSendNotification);

    // Restore preset selections saved in the processor state
    for (int i = 0; i < 3; ++i)
        selectedPresetPerPage[i] = audioProcessor.savedPresetIndex[i];

    // Restore the last active page (defaults to 0/Guitar if never saved)
    showPage (audioProcessor.savedPageIndex);
}

HadesAudioProcessorEditor::~HadesAudioProcessorEditor()
{
    btnGuitar.setLookAndFeel   (nullptr);
    btnBass.setLookAndFeel     (nullptr);
    btnCabinet.setLookAndFeel  (nullptr);
    btnSettings.setLookAndFeel (nullptr);
    presetBox.setLookAndFeel   (nullptr);
}

// ============================================================
//  Show Page
// ============================================================
void HadesAudioProcessorEditor::showPage (int index)
{
    currentPage = index;
    audioProcessor.savedPageIndex = index;

    guitarPage.setVisible   (index == 0);
    bassPage.setVisible     (index == 1);
    cabinetPage.setVisible  (index == 2);
    settingsPage.setVisible (index == 3);

    btnGuitar.setToggleState   (index == 0, juce::dontSendNotification);
    btnBass.setToggleState     (index == 1, juce::dontSendNotification);
    btnCabinet.setToggleState  (index == 2, juce::dontSendNotification);
    btnSettings.setToggleState (index == 3, juce::dontSendNotification);

    // Keep active_amp in sync so processBlock routes to the correct amp
    if (index == 0 || index == 1)
    {
        if (auto* param = audioProcessor.apvts.getParameter ("active_amp"))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 ((float)index));
            param->endChangeGesture();
        }
    }

    updatePresetBox (index);
    repaint();
}

// ============================================================
//  Update preset dropdown for each page
// ============================================================
void HadesAudioProcessorEditor::updatePresetBox (int pageIndex)
{
    presetBox.clear (juce::dontSendNotification);

    if (pageIndex == 0) // Guitar
    {
        presetBox.addItem ("HADES",     1);
        presetBox.addItem ("Clean",     2);
        presetBox.addItem ("Crunch",    3);
        presetBox.addItem ("High Gain", 4);
        presetBox.addItem ("Metal",     5);
        presetBox.addItem ("Lead",      6);
        presetBox.setSelectedId (selectedPresetPerPage[0] + 1, juce::dontSendNotification);
        presetBox.setVisible (true);
    }
    else if (pageIndex == 1) // Bass
    {
        presetBox.addItem ("HADES",     1);
        presetBox.addItem ("Warm",      2);
        presetBox.addItem ("Punchy",    3);
        presetBox.addItem ("Gritty",    4);
        presetBox.addItem ("Distorted", 5);
        presetBox.addItem ("Clean DI",  6);
        presetBox.setSelectedId (selectedPresetPerPage[1] + 1, juce::dontSendNotification);
        presetBox.setVisible (true);
    }
    else if (pageIndex == 2) // Cabinet
    {
        presetBox.addItem ("HADES",          1);
        presetBox.addItem ("4x12 Vintage",   2);
        presetBox.addItem ("4x12 Modern",    3);
        presetBox.addItem ("2x12 Open Back", 4);
        presetBox.addItem ("1x12 Combo",     5);
        presetBox.addItem ("2x12 Closed",    6);
        presetBox.setSelectedId (selectedPresetPerPage[2] + 1, juce::dontSendNotification);
        presetBox.setVisible (true);
    }
    else // Settings — no preset
    {
        presetBox.setVisible (false);
    }
}

// ============================================================
//  Paint — Nav bar background
// ============================================================
void HadesAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Pure black nav bar at top
    g.setColour (HadesColours::black);
    g.fillRect (0, 0, getWidth(), navHeight);

    // Subtle ember line under nav bar
    juce::ColourGradient borderGrad (
        HadesColours::ember.withAlpha (0.0f), 0.0f,              (float)navHeight,
        HadesColours::ember.withAlpha (0.7f), getWidth() * 0.5f, (float)navHeight,
        false
    );
    borderGrad.addColour (1.0, HadesColours::ember.withAlpha (0.0f));
    g.setGradientFill (borderGrad);
    g.fillRect (0, navHeight - 1, getWidth(), 2);

    // HADES logo/title on left
    g.setColour (HadesColours::ember);
    g.setFont (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::bold));
    g.drawText ("H A D E S", 16, 0, 120, navHeight, juce::Justification::centredLeft);
}

// ============================================================
//  Resized
// ============================================================
void HadesAudioProcessorEditor::resized()
{
    auto w = getWidth();
    auto h = getHeight();

    // Nav buttons — right side of nav bar
    int btnW    = 90;
    int btnH    = 30;
    int btnY    = (navHeight - btnH) / 2;
    int startX  = w - (btnW * 4) - (6 * 3) - 12;

    btnGuitar.setBounds   (startX,                   btnY, btnW, btnH);
    btnBass.setBounds     (startX + btnW + 6,        btnY, btnW, btnH);
    btnCabinet.setBounds  (startX + (btnW + 6) * 2, btnY, btnW, btnH);
    btnSettings.setBounds (startX + (btnW + 6) * 3, btnY, btnW, btnH);

    // Preset dropdown — spans from left edge of Cabinet button to right edge of Settings button
    int presetX = startX + (btnW + 6) * 2;                        // left edge of Cabinet
    int presetRight = startX + (btnW + 6) * 3 + btnW;             // right edge of Settings
    int presetW = presetRight - presetX;
    int presetH = 22;
    int ampTopY = navHeight + 20;
    int presetY = ampTopY - presetH - 2;
    presetBox.setBounds (presetX, presetY, presetW, presetH);

    // Pages fill everything below nav bar
    auto pageArea = juce::Rectangle<int> (0, navHeight, w, h - navHeight);
    guitarPage.setBounds   (pageArea);
    bassPage.setBounds     (pageArea);
    cabinetPage.setBounds  (pageArea);
    settingsPage.setBounds (pageArea);
}
