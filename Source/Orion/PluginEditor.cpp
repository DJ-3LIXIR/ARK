/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OrionSoundEQAudioProcessorEditor::OrionSoundEQAudioProcessorEditor (OrionSoundEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (1200, 800);
    setWantsKeyboardFocus(true);

    // === Style the buttons ===
    auto styleButton = [&](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xFF1A1A2E));
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF0DCDD4).withAlpha(0.3f));
        btn.setColour(juce::TextButton::textColourOffId,  juce::Colours::white.withAlpha(0.85f));
        btn.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xFF0DCDD4));
        addAndMakeVisible(btn);
    };

    styleButton(mainPageButton);
    mainPageButton.setClickingTogglesState(true);
    mainPageButton.setToggleState(true, juce::dontSendNotification);
    mainPageButton.setRadioGroupId(1001);
    mainPageButton.onClick = [this]()
    {
        showingMainPage = true;
        mainPageButton.setToggleState(true, juce::dontSendNotification);
        settingsButton.setToggleState(false, juce::dontSendNotification);
        setPageVisibility(true);
        repaint();
    };

    styleButton(settingsButton);
    settingsButton.setClickingTogglesState(true);
    settingsButton.setRadioGroupId(1001);
    settingsButton.onClick = [this]()
    {
        showingMainPage = false;
        settingsButton.setToggleState(true, juce::dontSendNotification);
        mainPageButton.setToggleState(false, juce::dontSendNotification);
        setPageVisibility(false);
        repaint();
    };

    // === Points Count Dropdown ===
    bandCountCombo.setLookAndFeel(&topBarLnf);
    bandCountCombo.addItem("1 Point",    1);
    bandCountCombo.addItem("3 Points",   2);
    bandCountCombo.addItem("5 Points",   3);
    bandCountCombo.addItem("8 Points",   4);
    bandCountCombo.addItem("12 Points",  5);
    bandCountCombo.addItem("16 Points",  6);
    bandCountCombo.addItem("32 Points",  7);
    bandCountCombo.setSelectedId(4, juce::dontSendNotification); // default 8 Points
    bandCountCombo.setTextWhenNothingSelected("Points");
    bandCountCombo.onChange = [this]()
    {
        const int counts[] = { 1, 3, 5, 8, 12, 16, 32 };
        int sel = bandCountCombo.getSelectedId();
        if (sel >= 1 && sel <= 7)
            updateBands(counts[sel - 1]);
        repaint();
    };
    addAndMakeVisible(bandCountCombo);

    // === Band Mode Dropdown ===
    bandModeCombo.setLookAndFeel(&topBarLnf);
    bandModeCombo.addItem("Single Band", 1);
    bandModeCombo.addItem("Multi Band",  2);
    bandModeCombo.setSelectedId(1, juce::dontSendNotification);
    bandModeCombo.setTextWhenNothingSelected("Band");
    addAndMakeVisible(bandModeCombo);

    // === Filter Dropdown (EQ filter types for selected dot) ===
    filterCombo.setLookAndFeel(&topBarLnf);
    filterCombo.addItem("Bell (Peak)",         1);
    filterCombo.addItem("Low Cut (HP)",        2);
    filterCombo.addItem("High Cut (LP)",       3);
    filterCombo.addItem("Low Shelf",           4);
    filterCombo.addItem("High Shelf",          5);
    filterCombo.addItem("Notch",               6);
    filterCombo.setSelectedId(1, juce::dontSendNotification);
    filterCombo.setTextWhenNothingSelected("Filter");
    filterCombo.onChange = [this]()
    {
        int sel = filterCombo.getSelectedId();

        // Items 1-6: change selected band's filter type only
        if (sel >= 1 && sel <= 6)
        {
            if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
            {
                pushUndoState();
                bands[selectedBandIndex].filterType = sel - 1;
                repaint();
            }
        }
    };
    addAndMakeVisible(filterCombo);

    // === Preset Dropdown (with nested submenus) ===
    presetCombo.setLookAndFeel(&topBarLnf);
    presetCombo.setTextWhenNothingSelected("Preset");
    rebuildPresetMenu();
    presetCombo.onChange = [this]()
    {
        int id = presetCombo.getSelectedId();
        if (id > 0 && id < 10000)
        {
            pushUndoState();
            applyPresetById(id);
        }
        else if (id >= 10000)
        {
            // User preset: IDs start at 10000
            int userIndex = id - 10000;
            pushUndoState();
            applyUserPreset(userIndex);
        }
    };
    addAndMakeVisible(presetCombo);

    // === Save Preset Button ===
    styleButton(savePresetButton);
    savePresetButton.onClick = [this]() { saveUserPreset(); };
    addAndMakeVisible(savePresetButton);

    // === Power Button (EQ Bypass) ===
    powerButton.setLookAndFeel(&powerLnf);
    powerButton.setClickingTogglesState(true);
    powerButton.setToggleState(false, juce::dontSendNotification); // false = EQ active
    powerButton.onClick = [this]()
    {
        eqBypassed = powerButton.getToggleState();
        powerLnf.bypassed = eqBypassed;
        audioProcessor.setBypassed(eqBypassed);
        repaint();
    };
    addAndMakeVisible(powerButton);

    // === Undo / Redo Buttons ===
    styleButton(undoButton);
    undoButton.onClick = [this]() { performUndo(); };
    addAndMakeVisible(undoButton);
    styleButton(redoButton);
    redoButton.onClick = [this]() { performRedo(); };
    addAndMakeVisible(redoButton);

    // === Bottom Bar Controls ===

    // Phase Mode dropdown
    phaseModeCombo.setLookAndFeel(&topBarLnf);
    phaseModeCombo.addItem("Natural Phase", 1);
    phaseModeCombo.addItem("Linear Phase", 2);
    phaseModeCombo.addItem("Zero Latency", 3);
    phaseModeCombo.setSelectedId(1, juce::dontSendNotification);
    phaseModeCombo.setTextWhenNothingSelected("Phase");
    addAndMakeVisible(phaseModeCombo);

    // Spectrum Analyzer toggle
    styleButton(spectrumButton);
    spectrumButton.setClickingTogglesState(true);
    addAndMakeVisible(spectrumButton);

    // MIDI Learn toggle
    styleButton(midiLearnButton);
    midiLearnButton.setClickingTogglesState(true);
    addAndMakeVisible(midiLearnButton);

    // === Knob Panel Controls ===
    auto setupKnob = [this](juce::Slider& knob, juce::Label& label, const juce::String& name,
                            double min, double max, double defaultVal, const juce::String& suffix)
    {
        knob.setLookAndFeel(&knobLnf);
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setRange(min, max);
        knob.setValue(defaultVal);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        if (suffix.isNotEmpty())
            knob.setTextValueSuffix(suffix);
        addAndMakeVisible(knob);

        label.setText(name, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.55f));
        label.setFont(juce::FontOptions(10.0f));
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };

    setupKnob(freqKnob, freqKnobLabel, "FREQ", 20.0, 20000.0, 1000.0, " Hz");
    freqKnob.setSkewFactorFromMidPoint(1000.0);
    freqKnob.setNumDecimalPlacesToDisplay(0);
    freqKnob.onValueChange = [this]()
    {
        if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
        {
            bands[selectedBandIndex].frequency = (float)freqKnob.getValue();
            repaint();
        }
    };

    setupKnob(gainKnob, gainKnobLabel, "GAIN", -24.0, 24.0, 0.0, " dB");
    gainKnob.setDoubleClickReturnValue(true, 0.0);
    gainKnob.setNumDecimalPlacesToDisplay(1);
    gainKnob.onValueChange = [this]()
    {
        if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
        {
            bands[selectedBandIndex].gainDB = (float)gainKnob.getValue();
            repaint();
        }
    };

    setupKnob(qKnob, qKnobLabel, "Q", 0.1, 10.0, 1.0, "");
    qKnob.setSkewFactorFromMidPoint(1.0);
    qKnob.setNumDecimalPlacesToDisplay(2);
    qKnob.onValueChange = [this]()
    {
        if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
        {
            bands[selectedBandIndex].Q = (float)qKnob.getValue();
            repaint();
        }
    };

    // Restore state from processor if DAW has loaded one, or editor was previously open
    if (audioProcessor.stateHasBeenLoaded || audioProcessor.editorStateInitialized)
    {
        int bandCount = audioProcessor.savedBandCount;

        // Find matching bandCountCombo ID for this band count
        const int counts[] = { 1, 3, 5, 8, 12, 16, 32 };
        int comboId = 3; // default to 5 points
        for (int i = 0; i < 7; ++i)
            if (counts[i] == bandCount) { comboId = i + 1; break; }

        bandCountCombo.setSelectedId(comboId, juce::dontSendNotification);
        updateBands(bandCount);

        // Restore per-band parameters
        for (int i = 0; i < bandCount && i < (int)bands.size(); ++i)
        {
            bands[i].frequency  = audioProcessor.savedBands[i].frequency;
            bands[i].gainDB     = audioProcessor.savedBands[i].gainDB;
            bands[i].Q          = audioProcessor.savedBands[i].Q;
            bands[i].filterType = audioProcessor.savedBands[i].filterType;
        }

        bandModeCombo.setSelectedId(audioProcessor.savedBandModeId, juce::dontSendNotification);

        // Rebuild preset menu in case user presets were loaded from DAW state
        if (!audioProcessor.userPresets.empty())
            rebuildPresetMenu();

        presetCombo.setSelectedId(audioProcessor.savedPresetComboId, juce::dontSendNotification);
        outputGainDB = audioProcessor.savedOutputGainDB;
    }
    else
    {
        // Initialize with 8 bands (first launch)
        updateBands(8);
    }

    // Start aurora animation at 30fps
    startTimerHz(30);
}

OrionSoundEQAudioProcessorEditor::~OrionSoundEQAudioProcessorEditor()
{
    bandCountCombo.setLookAndFeel(nullptr);
    bandModeCombo.setLookAndFeel(nullptr);
    filterCombo.setLookAndFeel(nullptr);
    presetCombo.setLookAndFeel(nullptr);
    phaseModeCombo.setLookAndFeel(nullptr);
    powerButton.setLookAndFeel(nullptr);
    freqKnob.setLookAndFeel(nullptr);
    gainKnob.setLookAndFeel(nullptr);
    qKnob.setLookAndFeel(nullptr);
}

//==============================================================================
void OrionSoundEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Space gradient background — more dark space, blue pushed toward bottom
    juce::ColourGradient gradient(
        juce::Colour(0xFF050208), 0.0f, 0.0f,                    // near-black at top
        juce::Colour(0xFF0DCDD4), 0.0f, (float)getHeight(),      // cyan at bottom
        false
    );
    gradient.addColour(0.45, juce::Colour(0xFF0A0614));  // stay dark until 45%
    gradient.addColour(0.70, juce::Colour(0xFF072A3A));  // transition to deep teal at 70%
    g.setGradientFill(gradient);
    g.fillAll();

    // ====== REAL NIGHT SKY — Orion & surrounding constellations ======
    // Coordinate system: RA 3.3h–8.2h (x, inverted), Dec -32°–+34° (y, inverted)
    // Maps real sky coordinates to screen pixels
    float W = (float)getWidth();
    float H = (float)getHeight();
    float raMin = 3.3f, raMax = 8.2f;
    float decMin = -32.0f, decMax = 34.0f;

    auto skyToScreen = [&](float ra, float dec) -> juce::Point<float> {
        float x = (raMax - ra) / (raMax - raMin) * W;
        float y = (decMax - dec) / (decMax - decMin) * H;
        return { x, y };
    };

    auto magToSize = [](float mag) -> float {
        return juce::jlimit(0.8f, 5.5f, 5.5f - mag * 1.1f);
    };

    // Dim random background stars for atmosphere
    juce::Random random(42);
    for (int i = 0; i < 120; ++i)
    {
        float x    = random.nextFloat() * W;
        float y    = random.nextFloat() * H * 0.85f;
        float size = random.nextFloat() * 0.9f + 0.2f;
        g.setColour(juce::Colours::white.withAlpha(random.nextFloat() * 0.25f + 0.15f));
        g.fillEllipse(x, y, size, size);
    }

    // Star data: { RA(hours), Dec(degrees), magnitude, colour }
    struct StarData { float ra; float dec; float mag; juce::uint32 col; };

    // === ORION ===
    StarData orion[] = {
        { 5.92f,   7.41f, 0.42f, 0xFFFFAA66 },  // Betelgeuse (red-orange)
        { 5.24f,  -8.20f, 0.13f, 0xFFCCDDFF },  // Rigel (blue-white)
        { 5.42f,   6.35f, 1.64f, 0xFFCCDDFF },  // Bellatrix (blue)
        { 5.53f,  -0.30f, 2.23f, 0xFFCCDDFF },  // Mintaka (belt)
        { 5.60f,  -1.20f, 1.69f, 0xFFCCDDFF },  // Alnilam (belt)
        { 5.68f,  -1.95f, 1.77f, 0xFFCCDDFF },  // Alnitak (belt)
        { 5.80f,  -9.67f, 2.09f, 0xFFCCDDFF },  // Saiph
        { 5.59f,   9.93f, 3.33f, 0xFFDDDDFF },  // Meissa (head)
    };

    // === TAURUS ===
    StarData taurus[] = {
        { 4.60f,  16.51f, 0.85f, 0xFFFFCC88 },  // Aldebaran (orange)
        { 5.44f,  28.61f, 1.65f, 0xFFCCDDFF },  // Elnath
        { 5.63f,  21.14f, 3.00f, 0xFFDDDDFF },  // Zeta Tauri
        { 4.33f,  15.96f, 3.53f, 0xFFDDDDFF },  // Hyadum I
        { 4.38f,  17.54f, 3.65f, 0xFFDDDDFF },  // Hyadum II
    };

    // === PLEIADES (tight cluster in Taurus) ===
    StarData pleiades[] = {
        { 3.79f,  24.11f, 2.87f, 0xFFCCDDFF },  // Alcyone
        { 3.82f,  24.05f, 3.63f, 0xFFCCDDFF },  // Atlas
        { 3.75f,  24.11f, 3.70f, 0xFFCCDDFF },  // Electra
        { 3.76f,  24.37f, 3.87f, 0xFFCCDDFF },  // Maia
        { 3.77f,  23.95f, 4.18f, 0xFFCCDDFF },  // Merope
        { 3.76f,  24.47f, 4.30f, 0xFFCCDDFF },  // Taygeta
    };

    // === GEMINI ===
    StarData gemini[] = {
        { 7.58f,  31.89f, 1.58f, 0xFFFFFFFF },  // Castor (white)
        { 7.76f,  28.03f, 1.14f, 0xFFFFCC88 },  // Pollux (orange)
        { 6.63f,  16.40f, 1.93f, 0xFFDDDDFF },  // Alhena
        { 6.38f,  22.51f, 3.36f, 0xFFDDDDFF },  // Mebsuta
        { 6.73f,  25.13f, 2.88f, 0xFFDDDDFF },  // Wasat
        { 7.07f,  30.25f, 3.53f, 0xFFDDDDFF },  // Kappa Gem
    };

    // === CANIS MAJOR ===
    StarData canisMajor[] = {
        { 6.75f, -16.72f,-1.46f, 0xFFFFFFFF },  // Sirius (BRIGHTEST)
        { 6.38f, -17.96f, 1.98f, 0xFFCCDDFF },  // Mirzam
        { 7.14f, -26.39f, 1.83f, 0xFFDDDDFF },  // Wezen
        { 6.98f, -28.97f, 1.50f, 0xFFCCDDFF },  // Adhara
        { 7.40f, -29.30f, 2.45f, 0xFFCCDDFF },  // Aludra
        { 6.94f, -24.18f, 3.02f, 0xFFDDDDFF },  // Sigma CMa
    };

    // === CANIS MINOR ===
    StarData canisMinor[] = {
        { 7.65f,   5.22f, 0.34f, 0xFFFFEECC },  // Procyon (yellow-white)
        { 7.45f,   8.29f, 2.84f, 0xFFCCDDFF },  // Gomeisa
    };

    // === LEPUS (below Orion) ===
    StarData lepus[] = {
        { 5.55f, -17.82f, 2.58f, 0xFFDDDDFF },  // Arneb
        { 5.47f, -20.76f, 2.84f, 0xFFFFCC88 },  // Nihal
        { 5.22f, -16.21f, 3.19f, 0xFFDDDDFF },  // Mu Lep
        { 5.09f, -22.37f, 3.31f, 0xFFDDDDFF },  // Epsilon Lep
    };

    // === ERIDANUS (right of Orion) ===
    StarData eridanus[] = {
        { 5.13f,  -5.09f, 2.79f, 0xFFDDDDFF },  // Cursa
        { 4.76f,  -3.25f, 3.54f, 0xFFDDDDFF },  // Tau4 Eri
        { 3.97f, -13.51f, 2.95f, 0xFFDDDDFF },  // Zaurak
    };

    // === MONOCEROS ===
    StarData monoceros[] = {
        { 7.69f,  -9.55f, 3.93f, 0xFFDDDDFF },  // Alpha Mon
        { 6.48f,  -7.03f, 3.74f, 0xFFDDDDFF },  // Beta Mon
    };

    // --- Draw constellation lines (faint) ---
    g.setColour(juce::Colours::white.withAlpha(0.08f));

    // Orion lines: shoulders → belt → feet, head
    auto drawLine = [&](StarData& a, StarData& b) {
        auto pa = skyToScreen(a.ra, a.dec);
        auto pb = skyToScreen(b.ra, b.dec);
        g.drawLine(pa.x, pa.y, pb.x, pb.y, 0.7f);
    };
    // Shoulders
    drawLine(orion[0], orion[2]);  // Betelgeuse – Bellatrix
    // Shoulders to belt
    drawLine(orion[0], orion[5]);  // Betelgeuse – Alnitak
    drawLine(orion[2], orion[3]);  // Bellatrix – Mintaka
    // Belt
    drawLine(orion[3], orion[4]);  // Mintaka – Alnilam
    drawLine(orion[4], orion[5]);  // Alnilam – Alnitak
    // Belt to feet
    drawLine(orion[5], orion[6]);  // Alnitak – Saiph
    drawLine(orion[3], orion[1]);  // Mintaka – Rigel
    // Head
    drawLine(orion[0], orion[7]);  // Betelgeuse – Meissa
    drawLine(orion[2], orion[7]);  // Bellatrix – Meissa

    // Taurus lines: V-shape of Hyades + horns
    drawLine(taurus[0], taurus[3]);  // Aldebaran – Hyadum I
    drawLine(taurus[3], taurus[4]);  // Hyadum I – Hyadum II
    drawLine(taurus[0], taurus[2]);  // Aldebaran – Zeta Tauri (horn)
    drawLine(taurus[4], taurus[1]);  // Hyadum II – Elnath (horn)

    // Gemini lines
    drawLine(gemini[0], gemini[1]);  // Castor – Pollux
    drawLine(gemini[0], gemini[5]);  // Castor – Kappa
    drawLine(gemini[1], gemini[4]);  // Pollux – Wasat
    drawLine(gemini[4], gemini[3]);  // Wasat – Mebsuta
    drawLine(gemini[3], gemini[2]);  // Mebsuta – Alhena

    // Canis Major lines
    drawLine(canisMajor[0], canisMajor[1]);  // Sirius – Mirzam
    drawLine(canisMajor[0], canisMajor[5]);  // Sirius – Sigma
    drawLine(canisMajor[5], canisMajor[2]);  // Sigma – Wezen
    drawLine(canisMajor[2], canisMajor[4]);  // Wezen – Aludra
    drawLine(canisMajor[2], canisMajor[3]);  // Wezen – Adhara

    // Canis Minor line
    drawLine(canisMinor[0], canisMinor[1]);  // Procyon – Gomeisa

    // Lepus lines
    drawLine(lepus[0], lepus[1]);  // Arneb – Nihal
    drawLine(lepus[0], lepus[2]);  // Arneb – Mu
    drawLine(lepus[1], lepus[3]);  // Nihal – Epsilon

    // --- Draw all constellation stars ---
    auto drawStars = [&](StarData* stars, int count) {
        for (int i = 0; i < count; ++i)
        {
            auto pos = skyToScreen(stars[i].ra, stars[i].dec);
            float sz = magToSize(stars[i].mag);
            juce::Colour c(stars[i].col);

            // Glow for bright stars (mag < 1.5)
            if (stars[i].mag < 1.5f)
            {
                float glowR = sz * 2.5f;
                g.setColour(c.withAlpha(0.08f));
                g.fillEllipse(pos.x - glowR, pos.y - glowR, glowR * 2, glowR * 2);
                float glowR2 = sz * 1.5f;
                g.setColour(c.withAlpha(0.15f));
                g.fillEllipse(pos.x - glowR2, pos.y - glowR2, glowR2 * 2, glowR2 * 2);
            }

            // Star dot
            g.setColour(c.withAlpha(0.9f));
            g.fillEllipse(pos.x - sz * 0.5f, pos.y - sz * 0.5f, sz, sz);
        }
    };

    drawStars(orion, 8);
    drawStars(taurus, 5);
    drawStars(pleiades, 6);
    drawStars(gemini, 6);
    drawStars(canisMajor, 6);
    drawStars(canisMinor, 2);
    drawStars(lepus, 4);
    drawStars(eridanus, 3);
    drawStars(monoceros, 2);

    // === Black Top Bar ===
    g.setColour(juce::Colours::black);
    g.fillRect(0.0f, 0.0f, (float)getWidth(), topBarHeight);

    // Subtle bottom edge
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.2f));
    g.drawLine(0.0f, topBarHeight, (float)getWidth(), topBarHeight, 1.0f);

    // Centered title
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::FontOptions(15.0f));
    g.drawText("Orion Sound EQ", 0, 0, getWidth(), (int)topBarHeight,
               juce::Justification::centred, false);

    // === Black Bottom Bar ===
    g.setColour(juce::Colours::black);
    g.fillRect(0.0f, (float)getHeight() - topBarHeight, (float)getWidth(), topBarHeight);

    // Subtle top edge on bottom bar
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.2f));
    g.drawLine(0.0f, (float)getHeight() - topBarHeight, (float)getWidth(), (float)getHeight() - topBarHeight, 1.0f);

    if (showingMainPage)
    {
        drawEQGrid(g);
        drawKnobPanel(g);
        drawOutputMeter(g);
    }
    else
    {
        drawSettingsPage(g);
    }
}

void OrionSoundEQAudioProcessorEditor::resized()
{
    int barH = (int)topBarHeight;
    int y = 4;
    int h = barH - 8;
    int x = 8;

    mainPageButton.setBounds(x, y, 90, h);
    x += 90 + 6;

    settingsButton.setBounds(x, y, 90, h);
    x += 90 + 6;

    bandCountCombo.setBounds(x, y, 160, h);
    x += 160 + 6;

    bandModeCombo.setBounds(x, y, 120, h);

    // Right side controls — laid out from the right edge inward
    int rx = getWidth() - 8;

    int redoW = 50;
    rx -= redoW;
    redoButton.setBounds(rx, y, redoW, h);
    rx -= 6;

    int undoW = 50;
    rx -= undoW;
    undoButton.setBounds(rx, y, undoW, h);
    rx -= 6;

    int saveW = 55;
    rx -= saveW;
    savePresetButton.setBounds(rx, y, saveW, h);
    rx -= 6;

    int presetW = 120;
    rx -= presetW;
    presetCombo.setBounds(rx, y, presetW, h);
    rx -= 6;

    int filterW = 140;
    rx -= filterW;
    filterCombo.setBounds(rx, y, filterW, h);
    rx -= 6;

    int powerW = h;  // Square button
    rx -= powerW;
    powerButton.setBounds(rx, y, powerW, h);

    // === Bottom Bar Layout ===
    int by = getHeight() - barH + 4;
    int bh = h;
    int bx = 8;

    // Phase Mode dropdown
    phaseModeCombo.setBounds(bx, by, 130, bh);
    bx += 130 + 10;

    // Spectrum Analyzer button
    spectrumButton.setBounds(bx, by, 80, bh);

    // MIDI Learn button (far right)
    int midiW = 90;
    midiLearnButton.setBounds(getWidth() - 8 - midiW, by, midiW, bh);

    // === Knob Panel Layout ===
    auto eqPanel = getPanelRect();
    int knobPanelW = 380;
    int knobPanelH = 160;
    float knobPanelBottom = eqPanel.getBottom() - 30.0f;
    int kpX = (int)(eqPanel.getX() + (eqPanel.getWidth() - knobPanelW) * 0.5f);
    int kpY = (int)(knobPanelBottom - knobPanelH);

    int labelH = 16;
    int topPad = 10;
    int botPad = 12;
    int knobSize = knobPanelH - topPad - labelH - botPad;
    int knobSpacing = (knobPanelW - knobSize * 3) / 4;

    for (int i = 0; i < 3; ++i)
    {
        int kx = kpX + knobSpacing + i * (knobSize + knobSpacing);
        juce::Slider* knob = (i == 0) ? &freqKnob : (i == 1) ? &gainKnob : &qKnob;
        juce::Label* label = (i == 0) ? &freqKnobLabel : (i == 1) ? &gainKnobLabel : &qKnobLabel;

        label->setVisible(true);
        label->setBounds(kx, kpY + topPad, knobSize, labelH);
        knob->setBounds(kx, kpY + topPad + labelH, knobSize, knobSize);
    }
}

//==============================================================================
// Coordinate mapping
//==============================================================================

float OrionSoundEQAudioProcessorEditor::freqToX(float freq, float panelX, float panelW) const
{
    float minLog = std::log10(20.0f);
    float maxLog = std::log10(20000.0f);
    float t = (std::log10(freq) - minLog) / (maxLog - minLog);
    return panelX + t * panelW;
}

float OrionSoundEQAudioProcessorEditor::dBToY(float dB, float panelY, float panelH) const
{
    float t = 1.0f - ((dB - dBMin) / (dBMax - dBMin));
    return panelY + t * panelH;
}

float OrionSoundEQAudioProcessorEditor::xToFreq(float x, float panelX, float panelW) const
{
    float minLog = std::log10(20.0f);
    float maxLog = std::log10(20000.0f);
    float t = (x - panelX) / panelW;
    t = juce::jlimit(0.0f, 1.0f, t);
    return std::pow(10.0f, minLog + t * (maxLog - minLog));
}

float OrionSoundEQAudioProcessorEditor::yToDB(float y, float panelY, float panelH) const
{
    float t = (y - panelY) / panelH;
    t = juce::jlimit(0.0f, 1.0f, t);
    float dB = dBMax - t * (dBMax - dBMin);
    return dB;
}

juce::Rectangle<float> OrionSoundEQAudioProcessorEditor::getPanelRect() const
{
    float margin = 40.0f;
    float panelX = margin;
    float panelY = topBarHeight + 10.0f;
    float panelW = getWidth() - margin - margin;
    float panelH = getHeight() - panelY - topBarHeight - 10.0f; // leave room for bottom bar
    return { panelX, panelY, panelW, panelH };
}

int OrionSoundEQAudioProcessorEditor::getHandleAtPosition(juce::Point<float> pos) const
{
    auto panel = getPanelRect();
    float hitRadius = bands.size() <= 8 ? 12.0f :
                      bands.size() <= 16 ? 9.0f : 7.0f;

    // Check in reverse so topmost drawn band is hit first
    for (int b = (int)bands.size() - 1; b >= 0; --b)
    {
        float hx = freqToX(bands[b].frequency, panel.getX(), panel.getWidth());
        float hy = dBToY(bands[b].gainDB, panel.getY(), panel.getHeight());
        float dist = pos.getDistanceFrom({ hx, hy });
        if (dist <= hitRadius)
            return b;
    }
    return -1;
}

//==============================================================================
// Mouse interaction
//==============================================================================

void OrionSoundEQAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    // Handle keybind editing clicks in Settings page
    if (!showingMainPage)
    {
        auto panel = getPanelRect();
        float midX = panel.getCentreX();
        float rightCol = midX + 20.0f;
        float rightW = panel.getRight() - rightCol - 30.0f;
        float rowH = 28.0f;
        float startY = panel.getY() + 58.0f + 25.0f;
        float keyX = rightCol + rightW * 0.5f;
        float keyW = rightW * 0.45f;

        // Check if click is on a keybind box
        for (int i = 0; i < 4; ++i)  // Only 4 editable keybinds
        {
            float y = startY + 28.0f + i * rowH;
            juce::Rectangle<float> keyBox(keyX, y, keyW, 20.0f);

            if (keyBox.contains(event.position))
            {
                editingKeybindIndex = i;
                repaint();
                return;
            }
        }

        // Check for Reset button
        float resetY = panel.getBottom() - 60.0f;
        float resetW = 120.0f;
        float resetX = midX - resetW * 0.5f;
        juce::Rectangle<float> resetBtn(resetX, resetY, resetW, 28.0f);

        if (resetBtn.contains(event.position))
        {
            // Reset to defaults
            customKeyBinds.deletePointKey = juce::KeyPress::backspaceKey;
            customKeyBinds.undoKey = 'z';
            customKeyBinds.undoKeyMods = juce::ModifierKeys::commandModifier;
            customKeyBinds.redoKey = 'z';
            customKeyBinds.redoKeyMods = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;
            editingKeybindIndex = -1;
            repaint();
            return;
        }

        return;
    }

    // Check if clicking on the output gain bar area (right side meter)
    auto panel = getPanelRect();
    float meterX = panel.getRight() + 4.0f;
    float meterRight = (float)getWidth() - 4.0f;

    if (event.position.x >= meterX - 8.0f && event.position.x <= meterRight + 8.0f
        && event.position.y >= panel.getY() && event.position.y <= panel.getBottom())
    {
        draggingOutputGain = true;
        // Convert Y to dB using inverse of meterDbToY
        float midY = panel.getY() + panel.getHeight() * 0.5f;
        float halfH = panel.getHeight() * 0.5f;
        float y = event.position.y;

        if (y <= midY)
        {
            float t = (midY - y) / halfH;
            outputGainDB = juce::jlimit(-24.0f, 24.0f, t * 24.0f);
        }
        else
        {
            float t = (y - midY) / halfH;
            float absDb = -18.0f * std::log(juce::jmax(0.001f, 1.0f - t));
            outputGainDB = juce::jlimit(-24.0f, 0.0f, -absDb);
        }
        repaint();
        return;
    }

    // Cmd+Click (macOS) / Ctrl+Click (Windows) = Add a new band at click position
    if (event.mods.isCommandDown())
    {
        auto panel = getPanelRect();
        if (panel.contains(event.position))
        {
            float freq = xToFreq(event.position.x, panel.getX(), panel.getWidth());
            float gain = yToDB(event.position.y, panel.getY(), panel.getHeight());
            addBandAtPosition(freq, gain);
            return;
        }
    }

    int clickedBand = getHandleAtPosition(event.position);
    dragBandIndex = clickedBand;

    // Select the band and sync knobs
    if (clickedBand >= 0 && clickedBand < (int)bands.size())
    {
        pushUndoState();  // Save state before any drag modification
        selectedBandIndex = clickedBand;
        // Sync knobs to selected band values (without triggering onChange)
        freqKnob.setValue(bands[selectedBandIndex].frequency, juce::dontSendNotification);
        gainKnob.setValue(bands[selectedBandIndex].gainDB, juce::dontSendNotification);
        qKnob.setValue(bands[selectedBandIndex].Q, juce::dontSendNotification);
        repaint();
    }
    else if (clickedBand < 0)
    {
        // Clicked empty space — check if it's inside the knob panel, if not deselect
        auto eqPanel = getPanelRect();
        int knobPanelW = 380;
        int knobPanelH = 160;
        float knobPanelBottom = eqPanel.getBottom() - 30.0f;
        float kpX = eqPanel.getX() + (eqPanel.getWidth() - knobPanelW) * 0.5f;
        float kpY = knobPanelBottom - knobPanelH;
        juce::Rectangle<float> knobPanelBounds(kpX, kpY, (float)knobPanelW, (float)knobPanelH);

        if (!knobPanelBounds.contains(event.position))
        {
            selectedBandIndex = -1;
            repaint();
        }
    }
}

void OrionSoundEQAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (!showingMainPage) return;

    // Handle output gain bar drag
    if (draggingOutputGain)
    {
        auto panel = getPanelRect();
        float midY = panel.getY() + panel.getHeight() * 0.5f;
        float halfH = panel.getHeight() * 0.5f;
        float y = event.position.y;

        if (y <= midY)
        {
            float t = (midY - y) / halfH;
            outputGainDB = juce::jlimit(-24.0f, 24.0f, t * 24.0f);
        }
        else
        {
            float t = juce::jlimit(0.0f, 0.999f, (y - midY) / halfH);
            float absDb = -18.0f * std::log(juce::jmax(0.001f, 1.0f - t));
            outputGainDB = juce::jlimit(-24.0f, 0.0f, -absDb);
        }
        repaint();
        return;
    }

    if (dragBandIndex < 0 || dragBandIndex >= (int)bands.size())
        return;

    auto panel = getPanelRect();

    // Map mouse position to frequency and gain
    float newFreq = xToFreq(event.position.x, panel.getX(), panel.getWidth());
    float newGain = yToDB(event.position.y, panel.getY(), panel.getHeight());

    // Clamp to valid ranges
    newFreq = juce::jlimit(20.0f, 20000.0f, newFreq);
    newGain = juce::jlimit(dBMin, dBMax, newGain);

    bands[dragBandIndex].frequency = newFreq;
    bands[dragBandIndex].gainDB    = newGain;

    // Sync knobs if dragging the selected band
    if (dragBandIndex == selectedBandIndex)
    {
        freqKnob.setValue(newFreq, juce::dontSendNotification);
        gainKnob.setValue(newGain, juce::dontSendNotification);
    }

    repaint();
}

void OrionSoundEQAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    dragBandIndex = -1;
    draggingOutputGain = false;
}

void OrionSoundEQAudioProcessorEditor::mouseMove(const juce::MouseEvent& event)
{
    if (!showingMainPage) { setMouseCursor(juce::MouseCursor::NormalCursor); return; }

    // Check if hovering over the output meter bar
    auto panel = getPanelRect();
    float meterX = panel.getRight() + 4.0f;
    float meterRight = (float)getWidth() - 4.0f;
    bool overMeter = event.position.x >= meterX - 8.0f && event.position.x <= meterRight + 8.0f
                     && event.position.y >= panel.getY() && event.position.y <= panel.getBottom();

    int newHovered = getHandleAtPosition(event.position);

    if (newHovered != hoveredBandIndex || overMeter)
    {
        hoveredBandIndex = newHovered;
        if (overMeter)
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        else if (hoveredBandIndex >= 0)
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        else
            setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void OrionSoundEQAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& event,
                                                       const juce::MouseWheelDetails& wheel)
{
    if (!showingMainPage) return;

    int bandIdx = getHandleAtPosition(event.position);
    if (bandIdx < 0 || bandIdx >= (int)bands.size())
        return;

    // Only push undo for first scroll event (not every tick)
    if (!draggingOutputGain && dragBandIndex < 0)
        pushUndoState();

    float delta = wheel.deltaY * 0.3f;
    bands[bandIdx].Q = juce::jlimit(0.1f, 10.0f, bands[bandIdx].Q + delta);

    if (bandIdx == selectedBandIndex)
        qKnob.setValue(bands[bandIdx].Q, juce::dontSendNotification);

    repaint();
}

// ==============================================================================
// Undo / Redo System
// ==============================================================================

void OrionSoundEQAudioProcessorEditor::pushUndoState()
{
    EQSnapshot snap;
    snap.bands = bands;
    snap.selectedBandIndex = selectedBandIndex;
    snap.bandModeId = bandModeCombo.getSelectedId();
    undoStack.push_back(snap);
    if ((int)undoStack.size() > maxUndoHistory)
        undoStack.erase(undoStack.begin());

    // Clear redo stack when new action happens
    redoStack.clear();
}

void OrionSoundEQAudioProcessorEditor::performUndo()
{
    if (undoStack.empty()) return;

    // Save current state to redo stack
    EQSnapshot current;
    current.bands = bands;
    current.selectedBandIndex = selectedBandIndex;
    current.bandModeId = bandModeCombo.getSelectedId();
    redoStack.push_back(current);

    // Restore from undo stack
    EQSnapshot& snap = undoStack.back();
    bands = snap.bands;
    selectedBandIndex = snap.selectedBandIndex;
    bandModeCombo.setSelectedId(snap.bandModeId, juce::dontSendNotification);

    // Update band count combo to match
    const int counts[] = { 1, 3, 5, 8, 12, 16, 32 };
    int bandCount = (int)bands.size();
    for (int i = 0; i < 7; ++i)
        if (counts[i] == bandCount) { bandCountCombo.setSelectedId(i + 1, juce::dontSendNotification); break; }

    // Sync knobs to selected band
    if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
    {
        freqKnob.setValue(bands[selectedBandIndex].frequency, juce::dontSendNotification);
        gainKnob.setValue(bands[selectedBandIndex].gainDB, juce::dontSendNotification);
        qKnob.setValue(bands[selectedBandIndex].Q, juce::dontSendNotification);
    }

    undoStack.pop_back();
    repaint();
}

void OrionSoundEQAudioProcessorEditor::performRedo()
{
    if (redoStack.empty()) return;

    // Save current state to undo stack
    EQSnapshot current;
    current.bands = bands;
    current.selectedBandIndex = selectedBandIndex;
    current.bandModeId = bandModeCombo.getSelectedId();
    undoStack.push_back(current);

    // Restore from redo stack
    EQSnapshot& snap = redoStack.back();
    bands = snap.bands;
    selectedBandIndex = snap.selectedBandIndex;
    bandModeCombo.setSelectedId(snap.bandModeId, juce::dontSendNotification);

    // Update band count combo to match
    const int counts[] = { 1, 3, 5, 8, 12, 16, 32 };
    int bandCount = (int)bands.size();
    for (int i = 0; i < 7; ++i)
        if (counts[i] == bandCount) { bandCountCombo.setSelectedId(i + 1, juce::dontSendNotification); break; }

    // Sync knobs to selected band
    if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
    {
        freqKnob.setValue(bands[selectedBandIndex].frequency, juce::dontSendNotification);
        gainKnob.setValue(bands[selectedBandIndex].gainDB, juce::dontSendNotification);
        qKnob.setValue(bands[selectedBandIndex].Q, juce::dontSendNotification);
    }

    redoStack.pop_back();
    repaint();
}

void OrionSoundEQAudioProcessorEditor::addBandAtPosition(float freq, float gainDB_)
{
    pushUndoState();

    EQBand newBand;
    newBand.frequency  = juce::jlimit(20.0f, 20000.0f, freq);
    newBand.gainDB     = juce::jlimit(-24.0f, 24.0f, gainDB_);
    newBand.Q          = 1.0f;
    newBand.filterType = 0;  // Bell by default
    newBand.colour     = getColourForBand((int)bands.size(), (int)bands.size() + 1);

    // Insert band in frequency-sorted order
    int insertIdx = (int)bands.size();
    for (int i = 0; i < (int)bands.size(); ++i)
    {
        if (bands[i].frequency > newBand.frequency)
        {
            insertIdx = i;
            break;
        }
    }
    bands.insert(bands.begin() + insertIdx, newBand);

    // Recolour all bands
    for (int i = 0; i < (int)bands.size(); ++i)
        bands[i].colour = getColourForBand(i, (int)bands.size());

    // Select the new band
    selectedBandIndex = insertIdx;
    freqKnob.setValue(newBand.frequency, juce::dontSendNotification);
    gainKnob.setValue(newBand.gainDB, juce::dontSendNotification);
    qKnob.setValue(newBand.Q, juce::dontSendNotification);

    repaint();
}

void OrionSoundEQAudioProcessorEditor::deleteSelectedBand()
{
    if (selectedBandIndex < 0 || selectedBandIndex >= (int)bands.size())
        return;
    if (bands.size() <= 1)
        return;  // Don't delete the last band

    pushUndoState();

    bands.erase(bands.begin() + selectedBandIndex);

    // Recolour all bands
    for (int i = 0; i < (int)bands.size(); ++i)
        bands[i].colour = getColourForBand(i, (int)bands.size());

    // Adjust selection
    if (selectedBandIndex >= (int)bands.size())
        selectedBandIndex = (int)bands.size() - 1;
    if (selectedBandIndex < 0)
        selectedBandIndex = -1;

    // Sync knobs
    if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
    {
        freqKnob.setValue(bands[selectedBandIndex].frequency, juce::dontSendNotification);
        gainKnob.setValue(bands[selectedBandIndex].gainDB, juce::dontSendNotification);
        qKnob.setValue(bands[selectedBandIndex].Q, juce::dontSendNotification);
    }

    repaint();
}

// ==============================================================================
// User Presets
// ==============================================================================

void OrionSoundEQAudioProcessorEditor::rebuildPresetMenu()
{
    // Remember current selection
    int currentId = presetCombo.getSelectedId();

    auto* rootMenu = presetCombo.getRootMenu();
    rootMenu->clear();

    // --- Basics ---
    juce::PopupMenu basicsMenu;
    basicsMenu.addItem(1,  "Default (Flat)");
    basicsMenu.addItem(2,  "Bass Boost");
    basicsMenu.addItem(3,  "Treble Boost");
    basicsMenu.addItem(4,  "Mid Scoop");
    basicsMenu.addItem(5,  "Vocal Presence");
    basicsMenu.addItem(6,  "Loudness");
    basicsMenu.addItem(7,  "Hi-Fi");
    basicsMenu.addItem(8,  "Telephone");
    basicsMenu.addItem(9,  "De-Esser");
    rootMenu->addSubMenu("Basics", basicsMenu);

    // --- Instrument ---
    juce::PopupMenu instrumentMenu;
    {
        juce::PopupMenu drumsMenu;
        drumsMenu.addItem(101, "Kick Punch");
        drumsMenu.addItem(102, "Kick Sub");
        drumsMenu.addItem(103, "Snare Crack");
        drumsMenu.addItem(104, "Snare Body");
        drumsMenu.addItem(105, "Tom Warmth");
        drumsMenu.addItem(106, "Overhead Shimmer");
        drumsMenu.addItem(107, "Room Tone");
        drumsMenu.addItem(108, "Hi-Hat Sizzle");
        instrumentMenu.addSubMenu("Drums", drumsMenu);

        juce::PopupMenu bassMenu;
        bassMenu.addItem(111, "Deep Sub");
        bassMenu.addItem(112, "Fingerstyle");
        bassMenu.addItem(113, "Slap Bass");
        bassMenu.addItem(114, "Picked Bass");
        bassMenu.addItem(115, "Bass DI Clean");
        bassMenu.addItem(116, "Bass Amp Warm");
        instrumentMenu.addSubMenu("Bass", bassMenu);

        juce::PopupMenu guitarMenu;
        guitarMenu.addItem(121, "Acoustic Bright");
        guitarMenu.addItem(122, "Acoustic Warm");
        guitarMenu.addItem(123, "Electric Clean");
        guitarMenu.addItem(124, "Electric Crunch");
        guitarMenu.addItem(125, "Jazz Smooth");
        guitarMenu.addItem(126, "Distorted Scoop");
        guitarMenu.addItem(127, "Nylon Classical");
        instrumentMenu.addSubMenu("Guitar", guitarMenu);

        juce::PopupMenu vocalMenu;
        vocalMenu.addItem(131, "Male Warmth");
        vocalMenu.addItem(132, "Male Presence");
        vocalMenu.addItem(133, "Female Presence");
        vocalMenu.addItem(134, "Female Air");
        vocalMenu.addItem(135, "Breathy");
        vocalMenu.addItem(136, "Powerful");
        vocalMenu.addItem(137, "Radio Ready");
        vocalMenu.addItem(138, "Nasal Fix");
        instrumentMenu.addSubMenu("Vocal", vocalMenu);

        juce::PopupMenu pianoMenu;
        pianoMenu.addItem(141, "Grand Warm");
        pianoMenu.addItem(142, "Grand Bright");
        pianoMenu.addItem(143, "Upright Character");
        pianoMenu.addItem(144, "Piano Pop");
        pianoMenu.addItem(145, "Piano Jazz");
        instrumentMenu.addSubMenu("Piano", pianoMenu);

        juce::PopupMenu stringsMenu;
        stringsMenu.addItem(151, "Orchestral Warm");
        stringsMenu.addItem(152, "Orchestral Bright");
        stringsMenu.addItem(153, "Violin Solo");
        stringsMenu.addItem(154, "Cello Body");
        stringsMenu.addItem(155, "String Ensemble");
        instrumentMenu.addSubMenu("Strings", stringsMenu);

        juce::PopupMenu synthMenu;
        synthMenu.addItem(161, "Synth Warm Pad");
        synthMenu.addItem(162, "Synth Aggressive");
        synthMenu.addItem(163, "Synth Glassy");
        synthMenu.addItem(164, "Synth Sub Bass");
        synthMenu.addItem(165, "Synth Lead");
        instrumentMenu.addSubMenu("Synth", synthMenu);
    }
    rootMenu->addSubMenu("Instrument", instrumentMenu);

    // --- Source/Mix ---
    juce::PopupMenu sourceMixMenu;
    {
        juce::PopupMenu podcastMenu;
        podcastMenu.addItem(201, "Voice Clarity");
        podcastMenu.addItem(202, "Broadcast");
        podcastMenu.addItem(203, "Warm Narrator");
        podcastMenu.addItem(204, "Interview");
        sourceMixMenu.addSubMenu("Podcast", podcastMenu);

        juce::PopupMenu voiceMenu;
        voiceMenu.addItem(211, "Voiceover Pro");
        voiceMenu.addItem(212, "Voice Bright");
        voiceMenu.addItem(213, "Voice Deep");
        voiceMenu.addItem(214, "Voice Telephone");
        sourceMixMenu.addSubMenu("Voice", voiceMenu);

        juce::PopupMenu acousticsMenu;
        acousticsMenu.addItem(221, "Acoustic Natural");
        acousticsMenu.addItem(222, "Acoustic Intimate");
        acousticsMenu.addItem(223, "Acoustic Wide");
        acousticsMenu.addItem(224, "Acoustic Bright");
        sourceMixMenu.addSubMenu("Acoustics", acousticsMenu);

        juce::PopupMenu electronicMenu;
        electronicMenu.addItem(231, "EDM Master");
        electronicMenu.addItem(232, "Lo-Fi");
        electronicMenu.addItem(233, "Trap Bass Heavy");
        electronicMenu.addItem(234, "Synthwave");
        electronicMenu.addItem(235, "House Clean");
        sourceMixMenu.addSubMenu("Electronic", electronicMenu);

        juce::PopupMenu liveMenu;
        liveMenu.addItem(241, "Live Vocal");
        liveMenu.addItem(242, "Live Guitar");
        liveMenu.addItem(243, "Live Drums OH");
        liveMenu.addItem(244, "Live Room Cleanup");
        liveMenu.addItem(245, "Live FOH PA");
        sourceMixMenu.addSubMenu("Live", liveMenu);

        juce::PopupMenu masteringMenu;
        masteringMenu.addItem(251, "Master Gentle");
        masteringMenu.addItem(252, "Master Bright");
        masteringMenu.addItem(253, "Master Warm");
        masteringMenu.addItem(254, "Master Loud");
        masteringMenu.addItem(255, "Master Reference");
        sourceMixMenu.addSubMenu("Mastering", masteringMenu);

        juce::PopupMenu balancedMenu;
        balancedMenu.addItem(261, "Balanced Neutral");
        balancedMenu.addItem(262, "Balanced Warm");
        balancedMenu.addItem(263, "Balanced Bright");
        sourceMixMenu.addSubMenu("Balanced", balancedMenu);
    }
    rootMenu->addSubMenu("Source/Mix", sourceMixMenu);

    // --- Corrective/Tone ---
    juce::PopupMenu correctiveMenu;
    {
        juce::PopupMenu warmMenu;
        warmMenu.addItem(301, "Warm Subtle");
        warmMenu.addItem(302, "Warm Heavy");
        warmMenu.addItem(303, "Warm Vintage");
        correctiveMenu.addSubMenu("Warm", warmMenu);

        juce::PopupMenu brightMenu;
        brightMenu.addItem(311, "Bright Subtle");
        brightMenu.addItem(312, "Bright Heavy");
        brightMenu.addItem(313, "Bright Airy");
        correctiveMenu.addSubMenu("Bright", brightMenu);

        juce::PopupMenu darkMenu;
        darkMenu.addItem(321, "Dark Subtle");
        darkMenu.addItem(322, "Dark Heavy");
        darkMenu.addItem(323, "Dark Lo-Fi");
        correctiveMenu.addSubMenu("Dark", darkMenu);

        juce::PopupMenu aggressiveMenu;
        aggressiveMenu.addItem(331, "Aggressive Mid");
        aggressiveMenu.addItem(332, "Aggressive Bright");
        aggressiveMenu.addItem(333, "Aggressive Scoop");
        correctiveMenu.addSubMenu("Aggressive", aggressiveMenu);

        juce::PopupMenu crispMenu;
        crispMenu.addItem(341, "Crisp Subtle");
        crispMenu.addItem(342, "Crisp Detailed");
        crispMenu.addItem(343, "Crisp Hi-Fi");
        correctiveMenu.addSubMenu("Crisp", crispMenu);

        juce::PopupMenu thickMenu;
        thickMenu.addItem(351, "Thick Subtle");
        thickMenu.addItem(352, "Thick Heavy");
        thickMenu.addItem(353, "Thick Wall");
        correctiveMenu.addSubMenu("Thick", thickMenu);
    }
    rootMenu->addSubMenu("Corrective/Tone", correctiveMenu);

    // --- User Presets (only if any exist) ---
    if (!audioProcessor.userPresets.empty())
    {
        juce::PopupMenu userMenu;
        for (int i = 0; i < (int)audioProcessor.userPresets.size(); ++i)
            userMenu.addItem(10000 + i, audioProcessor.userPresets[i].name);
        rootMenu->addSubMenu("User", userMenu);
    }

    // Restore selection if it was valid
    if (currentId > 0)
        presetCombo.setSelectedId(currentId, juce::dontSendNotification);
}

void OrionSoundEQAudioProcessorEditor::saveUserPreset()
{
    // Create an AlertWindow to prompt for preset name
    auto* alertWindow = new juce::AlertWindow("Save Preset",
                                               "Enter a name for your preset:",
                                               juce::MessageBoxIconType::NoIcon);
    alertWindow->addTextEditor("presetName", "My Preset", "Name:");
    alertWindow->addButton("Save", 1);
    alertWindow->addButton("Cancel", 0);

    // Style the alert window to match the theme
    alertWindow->setColour(juce::AlertWindow::backgroundColourId, juce::Colour(0xFF111122));
    alertWindow->setColour(juce::AlertWindow::textColourId, juce::Colours::white);
    alertWindow->setColour(juce::AlertWindow::outlineColourId, juce::Colour(0xFF0DCDD4));

    alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, alertWindow](int result)
        {
            if (result == 1)
            {
                juce::String name = alertWindow->getTextEditorContents("presetName").trim();
                if (name.isEmpty())
                    name = "Untitled";

                // Create a UserPreset from the current EQ state
                OrionSoundEQAudioProcessor::UserPreset up;
                up.name       = name;
                up.bandCount  = (int)bands.size();
                up.bandModeId = bandModeCombo.getSelectedId();

                for (int i = 0; i < (int)bands.size() && i < OrionSoundEQAudioProcessor::maxBands; ++i)
                {
                    up.bands[i].frequency  = bands[i].frequency;
                    up.bands[i].gainDB     = bands[i].gainDB;
                    up.bands[i].Q          = bands[i].Q;
                    up.bands[i].filterType = bands[i].filterType;
                }

                audioProcessor.userPresets.push_back(up);

                // Rebuild the preset menu so "User" category appears
                rebuildPresetMenu();

                // Select the new user preset in the dropdown
                int newId = 10000 + (int)audioProcessor.userPresets.size() - 1;
                presetCombo.setSelectedId(newId, juce::dontSendNotification);
            }

            delete alertWindow;
        }
    ));
}

void OrionSoundEQAudioProcessorEditor::applyUserPreset(int userPresetIndex)
{
    if (userPresetIndex < 0 || userPresetIndex >= (int)audioProcessor.userPresets.size())
        return;

    const auto& up = audioProcessor.userPresets[userPresetIndex];

    // Set the band count to match the saved preset
    int bandCount = up.bandCount;

    // Find matching bandCountCombo ID
    const int counts[] = { 1, 3, 5, 8, 12, 16, 32 };
    int comboId = 4; // default 8
    for (int i = 0; i < 7; ++i)
        if (counts[i] == bandCount) { comboId = i + 1; break; }

    bandCountCombo.setSelectedId(comboId, juce::dontSendNotification);
    updateBands(bandCount);

    // Restore band parameters
    for (int i = 0; i < bandCount && i < (int)bands.size(); ++i)
    {
        bands[i].frequency  = up.bands[i].frequency;
        bands[i].gainDB     = up.bands[i].gainDB;
        bands[i].Q          = up.bands[i].Q;
        bands[i].filterType = up.bands[i].filterType;
    }

    // Restore band mode
    bandModeCombo.setSelectedId(up.bandModeId, juce::dontSendNotification);

    // Sync knobs if there's a selected band
    if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
    {
        freqKnob.setValue(bands[selectedBandIndex].frequency, juce::dontSendNotification);
        gainKnob.setValue(bands[selectedBandIndex].gainDB, juce::dontSendNotification);
        qKnob.setValue(bands[selectedBandIndex].Q, juce::dontSendNotification);
        filterCombo.setSelectedId(bands[selectedBandIndex].filterType + 1, juce::dontSendNotification);
    }

    repaint();
}

// ==============================================================================
// Keyboard Handling
// ==============================================================================

bool OrionSoundEQAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    // If editing a keybind, capture the key
    if (editingKeybindIndex >= 0 && showingMainPage == false)  // Only in Settings page
    {
        switch (editingKeybindIndex)
        {
            case 0: // Delete Point
                customKeyBinds.deletePointKey = key.getKeyCode();
                break;
            case 1: // Add Point (Cmd+Click - can't rebind, skip)
                break;
            case 2: // Undo
                customKeyBinds.undoKey = key.getKeyCode();
                customKeyBinds.undoKeyMods = key.getModifiers().getRawFlags();
                break;
            case 3: // Redo
                customKeyBinds.redoKey = key.getKeyCode();
                customKeyBinds.redoKeyMods = key.getModifiers().getRawFlags();
                break;
        }
        editingKeybindIndex = -1;
        repaint();
        return true;
    }

    if (!showingMainPage) return false;

    // Use custom keybinds
    if (key.getKeyCode() == customKeyBinds.deletePointKey)
    {
        deleteSelectedBand();
        return true;
    }

    // Undo
    if (key.getKeyCode() == customKeyBinds.undoKey &&
        key.getModifiers().getRawFlags() == customKeyBinds.undoKeyMods)
    {
        performUndo();
        return true;
    }

    // Redo
    if (key.getKeyCode() == customKeyBinds.redoKey &&
        key.getModifiers().getRawFlags() == customKeyBinds.redoKeyMods)
    {
        performRedo();
        return true;
    }

    return false;
}

void OrionSoundEQAudioProcessorEditor::timerCallback()
{
    animPhase += 0.04f;   // controls aurora wave speed
    if (animPhase > juce::MathConstants<float>::twoPi * 100.0f)
        animPhase = 0.0f;

    // Smooth aurora level based on audio RMS
    float rms = audioProcessor.getCurrentRMSLevel();
    float target = (rms > 0.001f) ? 1.0f : 0.0f;  // threshold to ignore noise floor
    float smoothUp   = 0.15f;  // fast attack
    float smoothDown = 0.03f;  // slow fade out
    float rate = (target > auroraLevel) ? smoothUp : smoothDown;
    auroraLevel += rate * (target - auroraLevel);
    auroraLevel = juce::jlimit(0.0f, 1.0f, auroraLevel);

    // Push band data to processor (DSP + saved state)
    for (int i = 0; i < (int)bands.size(); ++i)
    {
        audioProcessor.setBandParameters(i, bands[i].frequency, bands[i].gainDB, bands[i].Q, bands[i].filterType);
        audioProcessor.savedBands[i] = { bands[i].frequency, bands[i].gainDB, bands[i].Q, bands[i].filterType };
    }
    audioProcessor.setNumActiveBands((int)bands.size());
    audioProcessor.setOutputGain(outputGainDB);
    audioProcessor.setBandMode(bandModeCombo.getSelectedId());
    audioProcessor.setSelectedBand(selectedBandIndex);

    // Keep saved UI state in sync for DAW recall
    audioProcessor.editorStateInitialized = true;
    audioProcessor.savedBandCount      = (int)bands.size();
    audioProcessor.savedBandModeId     = bandModeCombo.getSelectedId();
    audioProcessor.savedPresetComboId  = presetCombo.getSelectedId();
    audioProcessor.savedOutputGainDB   = outputGainDB;

    repaint();
}

//==============================================================================
// EQ Band engine
//==============================================================================

juce::Colour OrionSoundEQAudioProcessorEditor::getColourForBand(int index, int total)
{
    // Use golden-ratio hue spacing for unlimited distinct colours
    float hue = std::fmod(index * 0.618033988749895f, 1.0f);
    return juce::Colour::fromHSL(hue, 0.85f, 0.6f, 1.0f);
}

void OrionSoundEQAudioProcessorEditor::updateBands(int bandCount)
{
    bands.clear();

    float minLog = std::log10(20.0f);
    float maxLog = std::log10(20000.0f);
    float range  = maxLog - minLog;

    for (int i = 0; i < bandCount; ++i)
    {
        EQBand band;

        // Logarithmically spaced across 20Hz–20kHz
        float t = (i + 0.5f) / (float)bandCount;
        band.frequency = std::pow(10.0f, minLog + t * range);
        band.gainDB    = 0.0f;
        band.Q         = 1.0f;

        // First band = low shelf, last = high shelf, middle = bell
        if (bandCount == 1)
            band.filterType = 0;        // single band = bell
        else if (i == 0)
            band.filterType = 3;        // low shelf
        else if (i == bandCount - 1)
            band.filterType = 4;        // high shelf
        else
            band.filterType = 0;        // bell

        band.colour = getColourForBand(i, bandCount);
        bands.push_back(band);
    }
}

// ==============================================================================
// Preset System — 3-level hierarchical dropdowns
// ==============================================================================

void OrionSoundEQAudioProcessorEditor::applyPresetPoints(const std::vector<PresetPoint>& pts)
{
    if (pts.empty()) return;
    selectedBandIndex = -1;

    int N = (int)bands.size();
    int P = (int)pts.size();

    for (int i = 0; i < N; ++i)
    {
        float t = (P == 1) ? 0.0f : (float)i / (float)(N - 1) * (float)(P - 1);
        int lo = juce::jlimit(0, P - 1, (int)t);
        int hi = juce::jlimit(0, P - 1, lo + 1);
        float frac = t - (float)lo;

        float logFreqLo = std::log10(pts[lo].freq);
        float logFreqHi = std::log10(pts[hi].freq);
        bands[i].frequency  = std::pow(10.0f, logFreqLo + frac * (logFreqHi - logFreqLo));
        bands[i].gainDB     = pts[lo].gain + frac * (pts[hi].gain - pts[lo].gain);
        bands[i].Q          = pts[lo].Q    + frac * (pts[hi].Q    - pts[lo].Q);
        bands[i].filterType = (frac < 0.5f) ? pts[lo].filterType : pts[hi].filterType;
    }
    repaint();
}

void OrionSoundEQAudioProcessorEditor::applyPresetById(int presetId)
{
    // Switch on preset ID and apply corresponding preset
    std::vector<PresetPoint> pts;
    switch (presetId)
    {
        // Basics (1-9)
        case 1: for (auto& b : bands) { b.gainDB = 0.0f; b.Q = 1.0f; b.filterType = 0; } repaint(); return;
        case 2: pts = { {60,8,0.7f,3},{120,5,1,0},{400,0,1,0},{2500,-1,1,0},{10000,0,0.7f,4} }; break;
        case 3: pts = { {60,0,0.7f,3},{300,0,1,0},{2000,2,1,0},{5000,5,0.8f,0},{12000,7,0.7f,4} }; break;
        case 4: pts = { {80,3,0.7f,3},{250,-2,1.2f,0},{800,-6,0.8f,0},{2500,-3,1,0},{10000,4,0.7f,4} }; break;
        case 5: pts = { {100,-3,0.7f,1},{250,-2,1.5f,0},{1200,2,1,0},{3500,5,1.2f,0},{8000,3,0.7f,4} }; break;
        case 6: pts = { {60,6,0.7f,3},{200,2,1,0},{1000,0,1,0},{4000,3,0.8f,0},{12000,5,0.7f,4} }; break;
        case 7: pts = { {40,4,0.7f,3},{200,-1,1.2f,0},{1000,0,1,0},{5000,2,0.9f,0},{15000,5,0.7f,4} }; break;
        case 8: pts = { {300,0,1,1},{800,4,0.6f,0},{1500,6,0.8f,0},{2500,3,1,0},{3500,0,1,2} }; break;
        case 9: pts = { {80,0,0.7f,3},{500,0,1,0},{4000,-2,2,0},{6500,-6,2.5f,0},{9000,-3,1.5f,0} }; break;
        // Drums (101-108)
        case 101: pts = { {30,-3,0.7f,3},{60,6,0.8f,0},{100,2,1,0},{3500,4,1.5f,0},{10000,-2,0.7f,4} }; break;
        case 102: pts = { {30,8,0.6f,3},{60,5,0.7f,0},{200,-4,1.2f,0},{800,-2,1,0},{8000,-3,0.7f,4} }; break;
        case 103: pts = { {80,-2,0.7f,1},{200,3,1.2f,0},{900,-3,1.5f,0},{3000,6,1.2f,0},{10000,2,0.7f,4} }; break;
        case 104: pts = { {80,0,0.7f,3},{200,5,0.8f,0},{500,2,1,0},{2000,-1,1,0},{8000,-2,0.7f,4} }; break;
        case 105: pts = { {60,4,0.7f,3},{150,3,1,0},{400,-2,1.5f,0},{2500,2,1,0},{8000,-1,0.7f,4} }; break;
        case 106: pts = { {100,-4,0.7f,1},{400,-2,1,0},{2000,0,1,0},{6000,3,0.8f,0},{12000,5,0.7f,4} }; break;
        case 107: pts = { {60,2,0.7f,3},{200,0,1,0},{600,-3,0.8f,0},{3000,1,1,0},{10000,-2,0.7f,4} }; break;
        case 108: pts = { {200,-6,0.7f,1},{500,-3,1,0},{4000,0,1,0},{8000,4,1.2f,0},{14000,5,0.7f,4} }; break;
        // Bass (111-116)
        case 111: pts = { {30,7,0.6f,3},{80,4,0.8f,0},{250,-2,1.5f,0},{800,-4,1,0},{5000,-6,0.7f,2} }; break;
        case 112: pts = { {40,3,0.7f,3},{100,2,1,0},{500,0,1,0},{1200,3,1.2f,0},{6000,1,0.7f,4} }; break;
        case 113: pts = { {40,2,0.7f,3},{200,-3,1.5f,0},{800,-2,1,0},{2000,5,1.2f,0},{5000,3,0.8f,4} }; break;
        case 114: pts = { {50,3,0.7f,3},{150,0,1,0},{700,2,1.2f,0},{1500,4,1,0},{6000,-2,0.7f,4} }; break;
        case 115: pts = { {40,4,0.7f,3},{120,2,1,0},{400,0,1,0},{1000,1,0.8f,0},{4000,-3,0.7f,2} }; break;
        case 116: pts = { {50,5,0.7f,3},{150,3,0.8f,0},{500,0,1,0},{2000,-2,1.2f,0},{6000,-4,0.7f,4} }; break;
        // Guitar (121-127)
        case 121: pts = { {80,-2,0.7f,1},{200,0,1,0},{800,-1,1,0},{3000,3,1,0},{10000,5,0.7f,4} }; break;
        case 122: pts = { {80,2,0.7f,3},{200,3,0.8f,0},{600,0,1,0},{3000,-2,1.2f,0},{8000,-3,0.7f,4} }; break;
        case 123: pts = { {80,-3,0.7f,1},{300,0,1,0},{1200,2,1,0},{3500,3,0.8f,0},{8000,1,0.7f,4} }; break;
        case 124: pts = { {80,0,0.7f,3},{400,-3,1.5f,0},{1500,3,1,0},{3000,4,1.2f,0},{8000,2,0.7f,4} }; break;
        case 125: pts = { {60,2,0.7f,3},{250,2,0.8f,0},{800,0,1,0},{3000,-3,1.5f,0},{8000,-5,0.7f,4} }; break;
        case 126: pts = { {80,3,0.7f,3},{400,-4,1.5f,0},{800,-6,1,0},{2500,2,1.2f,0},{8000,3,0.7f,4} }; break;
        case 127: pts = { {80,0,0.7f,3},{200,3,0.8f,0},{500,1,1,0},{2000,2,1,0},{6000,-2,0.7f,4} }; break;
        // Vocal (131-138)
        case 131: pts = { {80,0,0.7f,3},{200,3,0.8f,0},{800,0,1,0},{2500,-1,1.2f,0},{8000,-2,0.7f,4} }; break;
        case 132: pts = { {80,-2,0.7f,1},{300,0,1,0},{1200,2,1,0},{3000,4,1.2f,0},{8000,2,0.7f,4} }; break;
        case 133: pts = { {100,-3,0.7f,1},{400,0,1,0},{2000,3,1,0},{4000,5,1.2f,0},{10000,2,0.7f,4} }; break;
        case 134: pts = { {100,-3,0.7f,1},{500,0,1,0},{3000,2,0.8f,0},{8000,4,0.7f,0},{14000,5,0.6f,4} }; break;
        case 135: pts = { {80,-2,0.7f,1},{250,0,1,0},{1500,1,0.8f,0},{5000,4,0.7f,0},{12000,3,0.7f,4} }; break;
        case 136: pts = { {80,0,0.7f,3},{250,2,1,0},{1000,3,1.2f,0},{3500,5,1,0},{8000,2,0.7f,4} }; break;
        case 137: pts = { {80,-4,0.7f,1},{250,2,1,0},{1200,3,1.2f,0},{4000,4,1,0},{12000,3,0.7f,4} }; break;
        case 138: pts = { {80,0,0.7f,3},{400,0,1,0},{800,-5,2.5f,0},{1200,-3,2,0},{6000,1,0.7f,4} }; break;
        // Piano (141-145)
        case 141: pts = { {50,2,0.7f,3},{200,2,0.8f,0},{500,0,1,0},{3000,-1,1,0},{8000,-2,0.7f,4} }; break;
        case 142: pts = { {50,0,0.7f,3},{300,0,1,0},{1500,1,1,0},{4000,3,0.8f,0},{12000,4,0.7f,4} }; break;
        case 143: pts = { {60,3,0.7f,3},{250,2,1,0},{700,0,1,0},{2000,2,1.5f,0},{6000,-1,0.7f,4} }; break;
        case 144: pts = { {60,-2,0.7f,1},{300,1,1,0},{1000,2,1,0},{3500,3,0.8f,0},{10000,2,0.7f,4} }; break;
        case 145: pts = { {60,2,0.7f,3},{200,3,0.8f,0},{600,0,1,0},{2500,-2,1.5f,0},{8000,-3,0.7f,4} }; break;
        // Strings (151-155)
        case 151: pts = { {80,2,0.7f,3},{300,2,0.8f,0},{800,0,1,0},{3000,-2,1.2f,0},{8000,-3,0.7f,4} }; break;
        case 152: pts = { {80,0,0.7f,3},{400,0,1,0},{2000,2,1,0},{5000,4,0.8f,0},{12000,3,0.7f,4} }; break;
        case 153: pts = { {150,-3,0.7f,1},{500,0,1,0},{1500,2,1.2f,0},{4000,4,1,0},{10000,2,0.7f,4} }; break;
        case 154: pts = { {60,3,0.7f,3},{150,3,0.8f,0},{500,0,1,0},{2000,-1,1.2f,0},{6000,-3,0.7f,4} }; break;
        case 155: pts = { {80,1,0.7f,3},{250,2,0.8f,0},{700,0,1,0},{3000,2,1,0},{10000,1,0.7f,4} }; break;
        // Synth (161-165)
        case 161: pts = { {40,3,0.7f,3},{200,2,0.8f,0},{600,0,1,0},{3000,-3,1.5f,0},{8000,-4,0.7f,4} }; break;
        case 162: pts = { {50,2,0.7f,3},{400,-3,1.5f,0},{1200,4,1.2f,0},{3500,5,1,0},{10000,3,0.7f,4} }; break;
        case 163: pts = { {80,-3,0.7f,1},{500,0,1,0},{2000,2,0.8f,0},{6000,4,0.7f,0},{14000,5,0.6f,4} }; break;
        case 164: pts = { {30,8,0.5f,3},{80,5,0.7f,0},{200,-2,1.5f,0},{500,-5,1,0},{3000,-8,0.7f,2} }; break;
        case 165: pts = { {80,-3,0.7f,1},{500,0,1,0},{1500,3,1.2f,0},{4000,4,1,0},{10000,2,0.7f,4} }; break;
        // Podcast (201-204)
        case 201: pts = { {80,-4,0.7f,1},{200,0,1,0},{1500,2,1,0},{4000,4,1.2f,0},{10000,1,0.7f,4} }; break;
        case 202: pts = { {60,-6,0.7f,1},{200,2,0.8f,0},{800,0,1,0},{3000,3,1,0},{8000,2,0.7f,4} }; break;
        case 203: pts = { {60,-3,0.7f,1},{200,3,0.8f,0},{600,1,1,0},{2500,-1,1.2f,0},{8000,-2,0.7f,4} }; break;
        case 204: pts = { {100,-5,0.7f,1},{350,0,1,0},{1200,2,1,0},{3500,3,1,0},{6000,0,0.7f,2} }; break;
        // Voice (211-214)
        case 211: pts = { {80,-4,0.7f,1},{250,2,0.8f,0},{1000,1,1,0},{3500,4,1.2f,0},{10000,2,0.7f,4} }; break;
        case 212: pts = { {80,-3,0.7f,1},{400,0,1,0},{2000,3,1,0},{5000,5,0.8f,0},{12000,3,0.7f,4} }; break;
        case 213: pts = { {60,3,0.7f,3},{200,3,0.8f,0},{600,0,1,0},{2500,-2,1.5f,0},{8000,-3,0.7f,4} }; break;
        case 214: pts = { {300,0,1,1},{800,4,0.6f,0},{1500,6,0.8f,0},{2500,3,1,0},{3500,0,1,2} }; break;
        // Acoustics (221-224)
        case 221: pts = { {50,0,0.7f,3},{200,1,0.8f,0},{800,0,1,0},{3000,1,1,0},{10000,0,0.7f,4} }; break;
        case 222: pts = { {60,2,0.7f,3},{250,2,0.8f,0},{700,0,1,0},{2500,-1,1.2f,0},{8000,-2,0.7f,4} }; break;
        case 223: pts = { {40,1,0.7f,3},{200,0,1,0},{600,-1,1,0},{4000,2,0.8f,0},{12000,3,0.7f,4} }; break;
        case 224: pts = { {60,-1,0.7f,3},{300,0,1,0},{1500,1,1,0},{5000,3,0.8f,0},{14000,4,0.7f,4} }; break;
        // Electronic (231-235)
        case 231: pts = { {30,4,0.6f,3},{100,2,0.8f,0},{500,-2,1.5f,0},{4000,3,0.8f,0},{12000,4,0.7f,4} }; break;
        case 232: pts = { {40,3,0.7f,3},{300,2,0.8f,0},{2000,-2,1,0},{6000,-5,0.7f,0},{10000,-8,0.7f,2} }; break;
        case 233: pts = { {30,10,0.5f,3},{80,6,0.7f,0},{300,-3,1.5f,0},{1000,-1,1,0},{8000,2,0.7f,4} }; break;
        case 234: pts = { {50,3,0.7f,3},{200,1,1,0},{800,-2,1.2f,0},{3000,2,0.8f,0},{10000,4,0.7f,4} }; break;
        case 235: pts = { {40,3,0.7f,3},{150,1,1,0},{600,0,1,0},{3500,2,0.8f,0},{12000,3,0.7f,4} }; break;
        // Live (241-245)
        case 241: pts = { {100,-5,0.7f,1},{300,0,1,0},{1500,3,1.2f,0},{4000,4,1,0},{8000,1,0.7f,4} }; break;
        case 242: pts = { {80,-3,0.7f,1},{250,0,1,0},{800,2,1,0},{3000,3,1.2f,0},{8000,0,0.7f,4} }; break;
        case 243: pts = { {80,-4,0.7f,1},{400,-2,1,0},{1500,0,1,0},{5000,3,0.8f,0},{12000,4,0.7f,4} }; break;
        case 244: pts = { {60,-6,0.7f,1},{250,-2,2,0},{500,-4,2.5f,0},{2000,0,1,0},{6000,1,0.7f,4} }; break;
        case 245: pts = { {40,-3,0.7f,1},{200,0,1,0},{800,-2,1.5f,0},{3000,2,1,0},{10000,1,0.7f,4} }; break;
        // Mastering (251-255)
        case 251: pts = { {40,1,0.7f,3},{200,0.5f,0.8f,0},{800,0,1,0},{4000,1,0.8f,0},{12000,1.5f,0.7f,4} }; break;
        case 252: pts = { {40,0,0.7f,3},{300,0,1,0},{2000,1,0.8f,0},{6000,2.5f,0.7f,0},{14000,3,0.7f,4} }; break;
        case 253: pts = { {40,2,0.7f,3},{200,1,0.8f,0},{800,0,1,0},{3000,-1,1,0},{10000,-1.5f,0.7f,4} }; break;
        case 254: pts = { {40,3,0.7f,3},{150,2,0.8f,0},{600,-1,1,0},{3000,2,0.8f,0},{12000,3,0.7f,4} }; break;
        case 255: pts = { {30,0,0.7f,3},{200,0,1,0},{1000,0,1,0},{5000,0,1,0},{15000,0,0.7f,4} }; break;
        // Balanced (261-263)
        case 261: pts = { {40,0,0.7f,3},{200,0,1,0},{800,0,1,0},{3000,0,1,0},{10000,0,0.7f,4} }; break;
        case 262: pts = { {50,2,0.7f,3},{200,1,0.8f,0},{700,0,1,0},{3000,-0.5f,1,0},{8000,-1,0.7f,4} }; break;
        case 263: pts = { {50,-0.5f,0.7f,3},{300,0,1,0},{1000,0.5f,1,0},{4000,1.5f,0.8f,0},{12000,2,0.7f,4} }; break;
        // Warm (301-303)
        case 301: pts = { {50,2,0.7f,3},{200,1,0.8f,0},{800,0,1,0},{3000,-1,1,0},{10000,-2,0.7f,4} }; break;
        case 302: pts = { {50,5,0.7f,3},{200,3,0.8f,0},{600,0,1,0},{2500,-3,1.5f,0},{8000,-5,0.7f,4} }; break;
        case 303: pts = { {60,4,0.7f,3},{250,2,0.8f,0},{700,0,1,0},{3000,-2,1.2f,0},{6000,-4,0.7f,2} }; break;
        // Bright (311-313)
        case 311: pts = { {50,-1,0.7f,3},{300,0,1,0},{1500,1,1,0},{5000,2,0.8f,0},{12000,3,0.7f,4} }; break;
        case 312: pts = { {50,-2,0.7f,3},{300,-1,1,0},{2000,3,1,0},{6000,5,0.8f,0},{14000,6,0.7f,4} }; break;
        case 313: pts = { {60,0,0.7f,3},{400,0,1,0},{3000,1,0.8f,0},{8000,3,0.7f,0},{16000,5,0.6f,4} }; break;
        // Dark (321-323)
        case 321: pts = { {50,1,0.7f,3},{200,0,1,0},{800,0,1,0},{4000,-2,1,0},{10000,-3,0.7f,4} }; break;
        case 322: pts = { {50,3,0.7f,3},{200,1,0.8f,0},{600,0,1,0},{3000,-4,1.2f,0},{8000,-7,0.7f,4} }; break;
        case 323: pts = { {40,4,0.7f,3},{300,2,0.8f,0},{1000,0,1,0},{4000,-5,0.7f,0},{6000,-10,0.7f,2} }; break;
        // Aggressive (331-333)
        case 331: pts = { {60,0,0.7f,3},{400,-2,1.5f,0},{1200,5,1.2f,0},{3000,6,1,0},{8000,2,0.7f,4} }; break;
        case 332: pts = { {60,0,0.7f,3},{300,-2,1.5f,0},{2000,4,1,0},{5000,6,0.8f,0},{12000,5,0.7f,4} }; break;
        case 333: pts = { {60,5,0.7f,3},{400,-5,1.5f,0},{800,-8,1,0},{2500,4,1.2f,0},{10000,5,0.7f,4} }; break;
        // Crisp (341-343)
        case 341: pts = { {60,0,0.7f,3},{400,0,1,0},{2000,1.5f,1,0},{5000,2.5f,0.8f,0},{12000,2,0.7f,4} }; break;
        case 342: pts = { {60,-1,0.7f,3},{500,0,1,0},{2500,3,1.2f,0},{6000,4,0.8f,0},{14000,3,0.7f,4} }; break;
        case 343: pts = { {40,2,0.7f,3},{200,-1,1.2f,0},{2000,2,1,0},{6000,4,0.7f,0},{15000,5,0.6f,4} }; break;
        // Thick (351-353)
        case 351: pts = { {50,2,0.7f,3},{150,2,0.8f,0},{400,1,1,0},{2000,-1,1,0},{8000,-2,0.7f,4} }; break;
        case 352: pts = { {40,5,0.6f,3},{150,4,0.7f,0},{400,2,1,0},{2000,-3,1.5f,0},{6000,-5,0.7f,4} }; break;
        case 353: pts = { {30,7,0.5f,3},{100,5,0.7f,0},{300,3,1,0},{1000,0,1,0},{4000,-4,0.7f,2} }; break;
    }
    if (!pts.empty()) applyPresetPoints(pts);
}
float OrionSoundEQAudioProcessorEditor::getBandGainAtFreq(const EQBand& band, float freq) const
{
    float octaves = std::log2(freq / band.frequency);

    switch (band.filterType)
    {
        case 0: // Bell
        {
            if (std::abs(band.gainDB) < 0.001f) return 0.0f;
            float bandwidth = 1.0f / band.Q;
            float x = octaves / (bandwidth * 0.5f);
            return band.gainDB * std::exp(-0.5f * x * x);
        }
        case 1: // Low Cut (HP) — shown as steep roll-off below cutoff
        {
            float slope = 4.0f * band.Q;
            float att = -48.0f * 0.5f * (1.0f - std::tanh(slope * octaves));
            return att;
        }
        case 2: // High Cut (LP) — shown as steep roll-off above cutoff
        {
            float slope = 4.0f * band.Q;
            float att = -48.0f * 0.5f * (1.0f + std::tanh(slope * octaves));
            return att;
        }
        case 3: // Low Shelf
        {
            if (std::abs(band.gainDB) < 0.001f) return 0.0f;
            float slope = 2.5f * band.Q;
            return band.gainDB * 0.5f * (1.0f - std::tanh(slope * octaves));
        }
        case 4: // High Shelf
        {
            if (std::abs(band.gainDB) < 0.001f) return 0.0f;
            float slope = 2.5f * band.Q;
            return band.gainDB * 0.5f * (1.0f + std::tanh(slope * octaves));
        }
        case 5: // Notch
        {
            float bandwidth = 1.0f / band.Q;
            float x = octaves / (bandwidth * 0.5f);
            float notchDepth = -24.0f;
            return notchDepth * std::exp(-0.5f * x * x);
        }
        default:
            return 0.0f;
    }
}

//==============================================================================
// Drawing
//==============================================================================

void OrionSoundEQAudioProcessorEditor::drawEQGrid(juce::Graphics& g)
{
    // === Panel area ===
    float margin = 40.0f;
    float panelX = margin;
    float panelY = topBarHeight + 10.0f;
    float panelW = getWidth()  - (margin * 2.0f);
    float panelH = getHeight() - panelY - margin;

    juce::Rectangle<float> panel(panelX, panelY, panelW, panelH);

    // Semi-transparent panel background
    g.setColour(juce::Colour(0xFF050510).withAlpha(0.20f));
    g.fillRoundedRectangle(panel, 6.0f);

    // Clip all EQ drawing to the panel
    g.saveState();
    g.reduceClipRegion(panel.toNearestIntEdges());

    // --- Draw band aurora fills and curves FIRST (behind grid) ---
    drawBandCurves(g, panel);

    g.restoreState();

    // === Frequency grid lines (vertical) ===
    const float freqs[] = { 20.f, 50.f, 100.f, 200.f, 500.f,
                             1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
    const int numFreqs = 10;

    g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.06f));
    for (int i = 0; i < numFreqs; ++i)
    {
        float x = freqToX(freqs[i], panelX, panelW);
        g.drawLine(x, panelY, x, panelY + panelH, 1.0f);
    }

    // Frequency labels
    g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.35f));
    g.setFont(juce::FontOptions(10.0f));
    const juce::String freqLabels[] = { "20", "50", "100", "200", "500",
                                        "1k", "2k", "5k", "10k", "20k" };
    for (int i = 0; i < numFreqs; ++i)
    {
        float x = freqToX(freqs[i], panelX, panelW);
        g.drawText(freqLabels[i],
                   (int)(x - 15), (int)(panelY + panelH - 18),
                   30, 14, juce::Justification::centred, false);
    }

    // === Gain grid lines (horizontal) ===
    const float dBLines[] = { 18.f, 12.f, 6.f, 0.f, -6.f, -12.f, -18.f };
    const int numDB = 7;

    for (int i = 0; i < numDB; ++i)
    {
        float y = dBToY(dBLines[i], panelY, panelH);
        bool isZero = (dBLines[i] == 0.0f);

        g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(isZero ? 0.18f : 0.06f));
        g.drawLine(panelX, y, panelX + panelW, y, isZero ? 1.5f : 1.0f);

        g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.30f));
        g.setFont(juce::FontOptions(10.0f));
        juce::String label = (dBLines[i] > 0 ? "+" : "") + juce::String((int)dBLines[i]);
        g.drawText(label, (int)panelX + 4, (int)(y - 7), 28, 14,
                   juce::Justification::centredLeft, false);
    }

    // Panel border
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.25f));
    g.drawRoundedRectangle(panel, 6.0f, 1.0f);

    // Draw band handles on top of everything
    drawBandHandles(g, panel);
}

void OrionSoundEQAudioProcessorEditor::drawBandCurves(juce::Graphics& g,
                                                       juce::Rectangle<float> panel)
{
    if (bands.empty()) return;

    float panelX = panel.getX();
    float panelY = panel.getY();
    float panelW = panel.getWidth();
    float panelH = panel.getHeight();
    float panelBottom = panelY + panelH;

    int numSamples = (int)panelW;
    if (numSamples < 2) return;

    float minLog = std::log10(20.0f);
    float maxLog = std::log10(20000.0f);

    // Pre-compute frequencies for each pixel column
    std::vector<float> pixelFreqs(numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        float t = (float)i / (float)(numSamples - 1);
        pixelFreqs[i] = std::pow(10.0f, minLog + t * (maxLog - minLog));
    }

    bool isMultiBand = (bandModeCombo.getSelectedId() == 2);

    // Pre-compute each band's individual gain curve
    std::vector<std::vector<float>> bandGains(bands.size(),
                                               std::vector<float>(numSamples, 0.0f));
    for (size_t b = 0; b < bands.size(); ++b)
        for (int i = 0; i < numSamples; ++i)
            bandGains[b][i] = getBandGainAtFreq(bands[b], pixelFreqs[i]);

    // Compute combined curve (sum of all bands) — used in single-band mode
    std::vector<float> combinedGain(numSamples, 0.0f);
    for (int i = 0; i < numSamples; ++i)
        for (size_t b = 0; b < bands.size(); ++b)
            combinedGain[i] += bandGains[b][i];

    // Pre-compute combined curve Y positions
    std::vector<float> curveY(numSamples);
    for (int i = 0; i < numSamples; ++i)
        curveY[i] = dBToY(combinedGain[i], panelY, panelH);

    // Pre-compute per-band curve Y positions (used in multi-band mode)
    std::vector<std::vector<float>> bandCurveY(bands.size(),
                                                std::vector<float>(numSamples));
    for (size_t b = 0; b < bands.size(); ++b)
        for (int i = 0; i < numSamples; ++i)
            bandCurveY[b][i] = dBToY(bandGains[b][i], panelY, panelH);

    // === Sort bands by frequency to determine regions ===
    std::vector<int> sortedIdx(bands.size());
    for (size_t i = 0; i < bands.size(); ++i)
        sortedIdx[i] = (int)i;
    std::sort(sortedIdx.begin(), sortedIdx.end(), [&](int a, int b) {
        return bands[a].frequency < bands[b].frequency;
    });

    // Compute X position for each band handle (sorted order)
    std::vector<float> bandX(bands.size());
    for (size_t i = 0; i < bands.size(); ++i)
        bandX[i] = freqToX(bands[i].frequency, panelX, panelW);

    // Compute region boundaries (midpoints between adjacent sorted bands)
    std::vector<float> regionLeft(bands.size());
    std::vector<float> regionRight(bands.size());

    for (size_t si = 0; si < sortedIdx.size(); ++si)
    {
        int bIdx = sortedIdx[si];

        // Left boundary
        if (si == 0)
            regionLeft[bIdx] = panelX;
        else
            regionLeft[bIdx] = (bandX[sortedIdx[si - 1]] + bandX[bIdx]) * 0.5f;

        // Right boundary
        if (si == sortedIdx.size() - 1)
            regionRight[bIdx] = panelX + panelW;
        else
            regionRight[bIdx] = (bandX[bIdx] + bandX[sortedIdx[si + 1]]) * 0.5f;
    }

    // === Draw per-band region fills ===
    for (size_t b = 0; b < bands.size(); ++b)
    {
        // Fills: full panel in multi-band, region-only in single-band
        float rLeft  = isMultiBand ? panelX : regionLeft[b];
        float rRight = isMultiBand ? (panelX + panelW) : regionRight[b];

        int iLeft  = juce::jlimit(0, numSamples - 1, (int)(rLeft - panelX));
        int iRight = juce::jlimit(0, numSamples - 1, (int)(rRight - panelX));
        if (iLeft >= iRight) continue;

        // Aurora always uses region boundaries so colors don't overlap
        int iLeftAurora  = juce::jlimit(0, numSamples - 1, (int)(regionLeft[b] - panelX));
        int iRightAurora = juce::jlimit(0, numSamples - 1, (int)(regionRight[b] - panelX));

        // Select which curve to use: per-band in multi-band, combined in single
        const std::vector<float>& activeCurve = isMultiBand ? bandCurveY[b] : curveY;

        juce::Colour bandCol       = bands[b].colour;
        juce::Colour bandColLight  = bandCol.brighter(0.4f);

        // --- Fill BELOW the curve, above 0dB line (band's exact colour) ---
        {
            float zeroY = dBToY(0.0f, panelY, panelH);

            juce::Path belowPath;
            // Clamp curve Y so fill only appears between curve and 0dB line
            float startY = juce::jmin(activeCurve[iLeft], zeroY);
            belowPath.startNewSubPath(panelX + (float)iLeft, startY);

            for (int i = iLeft + 1; i <= iRight; ++i)
            {
                float cy = juce::jmin(activeCurve[i], zeroY);
                belowPath.lineTo(panelX + (float)i, cy);
            }

            // Close down to 0dB line and back
            belowPath.lineTo(panelX + (float)iRight, zeroY);
            belowPath.lineTo(panelX + (float)iLeft,  zeroY);
            belowPath.closeSubPath();

            g.setColour(bandCol.darker(0.5f).withAlpha(0.30f));
            g.fillPath(belowPath);
        }

        // --- AURORA ABOVE CURVE (multiple layered curtains with internal wave brightness) ---
        // Three layers at different Y offsets with phase-shifted sine waves
        {
            // --- Wipe visibility function (three wipes, offset by thirds) ---
            // Shared across all aurora layers
            float wipeSpeed   = 0.065f;
            float wipeWidth   = panelW * 0.20f;
            float wipeMargin  = wipeWidth * 2.5f;
            float totalTravel = panelW + wipeMargin * 2.0f;

            float wipeCycle1   = std::fmod(animPhase * wipeSpeed, 1.0f);
            float wipeCenterX1 = (panelX + panelW + wipeMargin) - wipeCycle1 * totalTravel;

            float wipeCycle2   = std::fmod(animPhase * wipeSpeed + (1.0f / 3.0f), 1.0f);
            float wipeCenterX2 = (panelX + panelW + wipeMargin) - wipeCycle2 * totalTravel;

            float wipeCycle3   = std::fmod(animPhase * wipeSpeed + (2.0f / 3.0f), 1.0f);
            float wipeCenterX3 = (panelX + panelW + wipeMargin) - wipeCycle3 * totalTravel;

            auto getWipeVisibility = [&](float xPx) -> float
            {
                float dist1 = std::abs(xPx - wipeCenterX1) / wipeWidth;
                float erase1 = std::exp(-0.5f * dist1 * dist1);
                float dist2 = std::abs(xPx - wipeCenterX2) / wipeWidth;
                float erase2 = std::exp(-0.5f * dist2 * dist2);
                float dist3 = std::abs(xPx - wipeCenterX3) / wipeWidth;
                float erase3 = std::exp(-0.5f * dist3 * dist3);
                float totalErase = juce::jmin(1.0f, erase1 + erase2 + erase3);
                return 1.0f - totalErase;
            };

            // Layer parameters: { bottom dB, phase offset, alpha multiplier }
            // Layer 0: main curtain at +6dB, full brightness
            // Layer 1: second curtain at +8dB, phase shifted by π/3, slightly dimmer
            // Layer 2: third curtain at +10dB, phase shifted by 2π/3, dimmest
            struct AuroraLayer { float botDB; float phaseOffset; float alphaMult; };
            AuroraLayer layers[] = {
                { 6.0f,  0.0f,                                          1.0f  },
                { 8.0f,  juce::MathConstants<float>::pi / 3.0f,         0.55f },
                { 10.0f, juce::MathConstants<float>::pi * 2.0f / 3.0f,  0.30f }
            };

            for (int layer = 0; layer < 3; ++layer)
            {
                float auroraBot     = dBToY(layers[layer].botDB, panelY, panelH);
                float auroraTopBase = dBToY(24.0f, panelY, panelH);
                float layerPhase    = layers[layer].phaseOffset;
                float layerAlpha    = layers[layer].alphaMult;

                float waveAmplitude = (auroraBot - auroraTopBase) * 0.08f;

                juce::Path auroraRegion;

                // Bottom edge: animated sine wave (phase-shifted per layer)
                for (int i = iLeftAurora; i <= iRightAurora; ++i)
                {
                    float globalNormX = (float)i / juce::jmax(1.0f, (float)(numSamples - 1));
                    float w1 = std::sin(globalNormX * 8.0f  + animPhase * 0.3f + layerPhase);
                    float w2 = std::sin(globalNormX * 13.0f - animPhase * 0.2f + 1.5f + layerPhase);
                    float wave = (w1 + w2 * 0.4f) / 1.4f;
                    float y = auroraBot + wave * waveAmplitude;

                    if (i == iLeftAurora)
                        auroraRegion.startNewSubPath(panelX + (float)i, y);
                    else
                        auroraRegion.lineTo(panelX + (float)i, y);
                }

                // Top edge: mirrored sine wave (right to left)
                for (int i = iRightAurora; i >= iLeftAurora; --i)
                {
                    float globalNormX = (float)i / juce::jmax(1.0f, (float)(numSamples - 1));
                    float w1 = std::sin(globalNormX * 8.0f  + animPhase * 0.3f + layerPhase);
                    float w2 = std::sin(globalNormX * 13.0f - animPhase * 0.2f + 1.5f + layerPhase);
                    float wave = (w1 + w2 * 0.4f) / 1.4f;
                    float y = auroraTopBase - wave * waveAmplitude;

                    auroraRegion.lineTo(panelX + (float)i, y);
                }
                auroraRegion.closeSubPath();

                // --- Gradient strips with wipe, scaled by layer alpha ---
                {
                    float plus7Y  = dBToY(layers[layer].botDB + 1.0f, panelY, panelH);
                    float plus10Y = dBToY(layers[layer].botDB + 4.0f, panelY, panelH);
                    float plus14Y = dBToY(layers[layer].botDB + 8.0f, panelY, panelH);
                    float plus18Y = dBToY(18.0f, panelY, panelH);

                    g.saveState();
                    g.reduceClipRegion(auroraRegion);

                    float regionW = (float)(iRightAurora - iLeftAurora);
                    int stripW = 4;

                    for (int sx = iLeftAurora; sx < iRightAurora; sx += stripW)
                    {
                        int sw = juce::jmin(stripW, iRightAurora - sx);
                        float stripCenterX = panelX + (float)sx + (float)sw * 0.5f;

                        float wipeVis = getWipeVisibility(stripCenterX) * auroraLevel;
                        if (wipeVis < 0.01f) continue;

                        // Extend below auroraBot to cover the full sine wave swing
                        float waveBotExtend = auroraBot + waveAmplitude;

                        struct GradStop { float y; float alpha; };
                        GradStop stops[] = {
                            { auroraTopBase,  0.0f },
                            { plus18Y,       0.02f * layerAlpha },
                            { plus14Y,       0.06f * layerAlpha },
                            { plus10Y,       0.15f * layerAlpha },
                            { plus7Y,        0.35f * layerAlpha },
                            { auroraBot,     0.55f * layerAlpha },
                            { waveBotExtend, 0.55f * layerAlpha }
                        };

                        for (int gs = 0; gs < 6; ++gs)
                        {
                            float y0 = stops[gs].y;
                            float y1 = stops[gs + 1].y;
                            float avgAlpha = (stops[gs].alpha + stops[gs + 1].alpha) * 0.5f;
                            avgAlpha *= wipeVis;

                            if (avgAlpha > 0.002f)
                            {
                                juce::ColourGradient segGrad(
                                    bandColLight.withAlpha(stops[gs].alpha * wipeVis),     0.0f, y0,
                                    bandColLight.withAlpha(stops[gs + 1].alpha * wipeVis), 0.0f, y1,
                                    false
                                );
                                g.setGradientFill(segGrad);
                                g.fillRect(panelX + (float)sx, y0, (float)sw, y1 - y0);
                            }
                        }

                        // Internal wave brightness overlay (phase-shifted per layer)
                        float normX = (float)(sx - iLeftAurora) / juce::jmax(1.0f, regionW);
                        float wave1 = std::sin(normX * 2.8f  + animPhase * 0.25f + layerPhase);
                        float wave2 = std::sin(normX * 5.1f  - animPhase * 0.18f + 1.3f + layerPhase);
                        float wave3 = std::sin(normX * 1.2f  + animPhase * 0.12f + 3.7f + layerPhase);
                        float brightness = (wave1 + wave2 * 0.6f + wave3 * 0.4f) / 2.0f;
                        float overlayAlpha = brightness * 0.06f * wipeVis * layerAlpha;

                        if (overlayAlpha > 0.0f)
                        {
                            g.setColour(bandColLight.withAlpha(overlayAlpha));
                            g.fillRect(panelX + (float)sx, auroraTopBase, (float)sw, auroraBot - auroraTopBase);
                        }
                    }

                    g.restoreState();
                }
            } // end layer loop
        }

        // --- Fill between 0dB and curve when curve is BELOW 0dB (lighter colour) ---
        {
            float zeroY = dBToY(0.0f, panelY, panelH);

            juce::Path negPath;
            // Use max so we only trace portions below 0dB
            float startY = juce::jmax(activeCurve[iLeft], zeroY);
            negPath.startNewSubPath(panelX + (float)iLeft, startY);

            for (int i = iLeft + 1; i <= iRight; ++i)
            {
                float cy = juce::jmax(activeCurve[i], zeroY);
                negPath.lineTo(panelX + (float)i, cy);
            }

            // Close back along 0dB line
            negPath.lineTo(panelX + (float)iRight, zeroY);
            negPath.lineTo(panelX + (float)iLeft,  zeroY);
            negPath.closeSubPath();

            g.setColour(bandColLight.withAlpha(0.14f));
            g.fillPath(negPath);
        }
    }

    // === White aurora streaks across all three layers ===
    // Layer 1: divider streaks at band borders, Layers 2 & 3: randomly placed
    // All attached to their respective layer's sine wave edge
    {
        // Shared tri-wipe
        float wipeSpeedS   = 0.065f;
        float wipeWidthS   = panelW * 0.20f;
        float wipeMarginS  = wipeWidthS * 2.5f;
        float totalTravelS = panelW + wipeMarginS * 2.0f;

        float wipeCycleS1   = std::fmod(animPhase * wipeSpeedS, 1.0f);
        float wipeCenterXS1 = (panelX + panelW + wipeMarginS) - wipeCycleS1 * totalTravelS;
        float wipeCycleS2   = std::fmod(animPhase * wipeSpeedS + (1.0f / 3.0f), 1.0f);
        float wipeCenterXS2 = (panelX + panelW + wipeMarginS) - wipeCycleS2 * totalTravelS;
        float wipeCycleS3   = std::fmod(animPhase * wipeSpeedS + (2.0f / 3.0f), 1.0f);
        float wipeCenterXS3 = (panelX + panelW + wipeMarginS) - wipeCycleS3 * totalTravelS;

        auto getStreakWipeVis = [&](float xPos) -> float
        {
            float d1 = std::abs(xPos - wipeCenterXS1) / wipeWidthS;
            float e1 = std::exp(-0.5f * d1 * d1);
            float d2 = std::abs(xPos - wipeCenterXS2) / wipeWidthS;
            float e2 = std::exp(-0.5f * d2 * d2);
            float d3 = std::abs(xPos - wipeCenterXS3) / wipeWidthS;
            float e3 = std::exp(-0.5f * d3 * d3);
            return (1.0f - juce::jmin(1.0f, e1 + e2 + e3)) * auroraLevel;
        };

        // Per-layer info matching aurora layers
        struct StreakLayer { float botDB; float phaseOff; float alphaMult; };
        StreakLayer sLayers[] = {
            { 6.0f,  0.0f,                                          1.0f  },
            { 8.0f,  juce::MathConstants<float>::pi / 3.0f,         0.55f },
            { 10.0f, juce::MathConstants<float>::pi * 2.0f / 3.0f,  0.30f }
        };

        for (int sl = 0; sl < 3; ++sl)
        {
            float layerBotY  = dBToY(sLayers[sl].botDB, panelY, panelH);
            float layerTopY  = dBToY(24.0f, panelY, panelH);
            float layerPhase = sLayers[sl].phaseOff;
            float layerAMult = sLayers[sl].alphaMult;
            float waveAmp    = (layerBotY - layerTopY) * 0.08f;

            // Wave bottom edge matching this layer's sine wave
            auto getLayerWaveBot = [&](float xPixel) -> float
            {
                float globalNormX = (xPixel - panelX) / juce::jmax(1.0f, panelW);
                float w1 = std::sin(globalNormX * 8.0f  + animPhase * 0.3f + layerPhase);
                float w2 = std::sin(globalNormX * 13.0f - animPhase * 0.2f + 1.5f + layerPhase);
                float wave = (w1 + w2 * 0.4f) / 1.4f;
                return layerBotY + wave * waveAmp;
            };

            if (sl == 0)
            {
                // Layer 1: streaks at band divider borders
                if (sortedIdx.size() > 1)
                {
                    for (size_t si = 0; si + 1 < sortedIdx.size(); ++si)
                    {
                        int leftBand  = sortedIdx[si];
                        int rightBand = sortedIdx[si + 1];
                        float borderX = (bandX[leftBand] + bandX[rightBand]) * 0.5f;

                        const int numStreaks = 5;
                        for (int s = 0; s < numStreaks; ++s)
                        {
                            float streakPhase = animPhase * (0.15f + s * 0.06f) + s * 1.4f;
                            float xOffset = std::sin(streakPhase) * 12.0f + (s - 2) * 4.0f;
                            float streakX = borderX + xOffset;

                            float wipeVis = getStreakWipeVis(streakX);
                            if (wipeVis < 0.01f) continue;

                            float streakBot = getLayerWaveBot(streakX);
                            float heightNorm = 0.4f + 0.5f * (0.5f + 0.5f * std::sin(streakPhase * 0.7f + s));
                            float streakTop  = layerTopY + (1.0f - heightNorm) * (streakBot - layerTopY);
                            float streakW = 1.0f + std::abs(std::sin(streakPhase * 0.3f)) * 1.5f;

                            float distFromCenter = std::abs(s - 2) / 2.0f;
                            float baseAlpha = 0.18f - distFromCenter * 0.06f;
                            float alphaPulse = 0.5f + 0.5f * std::sin(animPhase * 0.2f + s * 0.9f);
                            float alpha = baseAlpha * (0.6f + alphaPulse * 0.4f) * wipeVis * layerAMult;

                            juce::ColourGradient streakGrad(
                                juce::Colours::white.withAlpha(alpha),  streakX, streakBot,
                                juce::Colours::white.withAlpha(0.0f),   streakX, streakTop,
                                false
                            );
                            g.setGradientFill(streakGrad);
                            g.fillRect(streakX - streakW * 0.5f, streakTop, streakW, streakBot - streakTop);
                        }
                    }
                }
            }
            else
            {
                // Layers 2 & 3: randomly placed streaks, attached to this layer's wave
                int numRandomStreaks = 8 + sl * 4; // 12 for layer 2, 16 for layer 3
                for (int rs = 0; rs < numRandomStreaks; ++rs)
                {
                    // Deterministic pseudo-random X position using golden ratio hash
                    float hash = std::fmod((float)(rs * 127 + sl * 311) * 0.6180339887f, 1.0f);
                    float streakX = panelX + hash * panelW;

                    // Gentle animated sway
                    float streakPhase = animPhase * (0.12f + rs * 0.04f) + rs * 2.1f + sl * 5.3f;
                    float xSway = std::sin(streakPhase) * 8.0f;
                    streakX += xSway;

                    if (streakX < panelX || streakX > panelX + panelW) continue;

                    float wipeVis = getStreakWipeVis(streakX);
                    if (wipeVis < 0.01f) continue;

                    // Attach to this layer's sine wave bottom edge
                    float streakBot = getLayerWaveBot(streakX);
                    float heightNorm = 0.3f + 0.6f * (0.5f + 0.5f * std::sin(streakPhase * 0.5f + rs));
                    float streakTop  = layerTopY + (1.0f - heightNorm) * (streakBot - layerTopY);
                    float streakW = 0.8f + std::abs(std::sin(streakPhase * 0.25f)) * 1.2f;

                    float baseAlpha = 0.14f;
                    float alphaPulse = 0.5f + 0.5f * std::sin(animPhase * 0.18f + rs * 1.3f);
                    float alpha = baseAlpha * (0.5f + alphaPulse * 0.5f) * wipeVis * layerAMult;

                    juce::ColourGradient streakGrad(
                        juce::Colours::white.withAlpha(alpha),  streakX, streakBot,
                        juce::Colours::white.withAlpha(0.0f),   streakX, streakTop,
                        false
                    );
                    g.setGradientFill(streakGrad);
                    g.fillRect(streakX - streakW * 0.5f, streakTop, streakW, streakBot - streakTop);
                }
            }
        }
    }

    // === Draw curve(s) ===
    if (isMultiBand)
    {
        // Multi-band: each band draws its own individual curve in its own colour
        for (size_t b = 0; b < bands.size(); ++b)
        {
            juce::Path bandPath;
            bandPath.startNewSubPath(panelX, bandCurveY[b][0]);
            for (int i = 1; i < numSamples; ++i)
                bandPath.lineTo(panelX + (float)i, bandCurveY[b][i]);

            // Per-band curve glow
            g.setColour(bands[b].colour.withAlpha(0.12f));
            g.strokePath(bandPath, juce::PathStrokeType(4.0f));

            // Per-band curve line
            g.setColour(bands[b].colour.withAlpha(0.85f));
            g.strokePath(bandPath, juce::PathStrokeType(1.5f));
        }
    }
    else
    {
        // Single-band: one combined white curve
        juce::Path combinedPath;
        combinedPath.startNewSubPath(panelX, curveY[0]);
        for (int i = 1; i < numSamples; ++i)
            combinedPath.lineTo(panelX + (float)i, curveY[i]);

        // Combined curve glow
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.strokePath(combinedPath, juce::PathStrokeType(4.0f));

        // Combined curve line — bright white
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.strokePath(combinedPath, juce::PathStrokeType(1.5f));
    }
}

void OrionSoundEQAudioProcessorEditor::drawBandHandles(juce::Graphics& g,
                                                        juce::Rectangle<float> panel)
{
    float panelX = panel.getX();
    float panelY = panel.getY();
    float panelW = panel.getWidth();
    float panelH = panel.getHeight();

    // Scale handle size based on band count
    float handleRadius = bands.size() <= 8 ? 7.0f :
                          bands.size() <= 16 ? 5.0f : 4.0f;

    for (size_t b = 0; b < bands.size(); ++b)
    {
        float x = freqToX(bands[b].frequency, panelX, panelW);
        float y = dBToY(bands[b].gainDB, panelY, panelH);

        bool isHovered = ((int)b == dragBandIndex || (int)b == hoveredBandIndex);
        bool isSelected = ((int)b == selectedBandIndex);
        bool isActive = isHovered || isSelected;
        float r = isActive ? handleRadius + 2.0f : handleRadius;

        // Determine band colour (grey when bypassed)
        juce::Colour bandCol = eqBypassed
            ? juce::Colour(0xFF555555)
            : bands[b].colour;

        // Selection ring (pulsing glow for selected band)
        if (isSelected && !eqBypassed)
        {
            float pulseAlpha = 0.3f + 0.15f * std::sin(animPhase * 2.0f);
            g.setColour(bandCol.withAlpha(pulseAlpha));
            g.fillEllipse(x - r - 8.0f, y - r - 8.0f, (r + 8.0f) * 2, (r + 8.0f) * 2);

            // White selection ring
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.drawEllipse(x - r - 4.0f, y - r - 4.0f, (r + 4.0f) * 2, (r + 4.0f) * 2, 1.5f);
        }

        // Outer glow
        float glowExtra = isActive && !eqBypassed ? 5.0f : 3.0f;
        float glowAlpha = eqBypassed ? 0.1f : (isActive ? 0.45f : 0.25f);
        g.setColour(bandCol.withAlpha(glowAlpha));
        g.fillEllipse(x - r - glowExtra, y - r - glowExtra,
                      (r + glowExtra) * 2, (r + glowExtra) * 2);

        // Filled circle
        g.setColour(bandCol);
        g.fillEllipse(x - r, y - r, r * 2, r * 2);

        // White border (dimmer when bypassed)
        float borderAlpha = eqBypassed ? 0.3f : (isActive ? 1.0f : 0.7f);
        g.setColour(juce::Colours::white.withAlpha(borderAlpha));
        g.drawEllipse(x - r, y - r, r * 2, r * 2, isActive ? 1.8f : 1.2f);
    }

    // === Info box for selected band ===
    if (selectedBandIndex >= 0 && selectedBandIndex < (int)bands.size())
    {
        auto& band = bands[selectedBandIndex];
        float bx = freqToX(band.frequency, panelX, panelW);
        float by = dBToY(band.gainDB, panelY, panelH);

        // Format values
        juce::String freqStr = (band.frequency >= 1000.0f)
            ? juce::String(band.frequency / 1000.0f, 1) + " kHz"
            : juce::String((int)band.frequency) + " Hz";
        juce::String gainStr = (band.gainDB >= 0 ? "+" : "") + juce::String(band.gainDB, 1) + " dB";
        juce::String qStr    = "Q " + juce::String(band.Q, 2);

        // Box dimensions
        float boxW = 90.0f;
        float boxH = 48.0f;
        float boxX = bx - boxW * 0.5f;
        float boxY = by - handleRadius - 8.0f - boxH; // above the handle

        // Keep box inside panel
        boxX = juce::jlimit(panelX + 2.0f, panelX + panelW - boxW - 2.0f, boxX);
        if (boxY < panelY + 2.0f)
            boxY = by + handleRadius + 10.0f; // flip below if too high

        // Dark rounded box
        g.setColour(juce::Colour(0xFF0A0A14).withAlpha(0.92f));
        g.fillRoundedRectangle(boxX, boxY, boxW, boxH, 6.0f);
        g.setColour(band.colour.withAlpha(0.5f));
        g.drawRoundedRectangle(boxX, boxY, boxW, boxH, 6.0f, 1.0f);

        // Text
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawText(freqStr, (int)boxX, (int)boxY + 3,  (int)boxW, 14, juce::Justification::centred, false);
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawText(gainStr, (int)boxX, (int)boxY + 17, (int)boxW, 14, juce::Justification::centred, false);
        g.drawText(qStr,    (int)boxX, (int)boxY + 31, (int)boxW, 14, juce::Justification::centred, false);
    }
}

//==============================================================================
// Knob Panel (centered in bottom half of EQ grid)
//==============================================================================
void OrionSoundEQAudioProcessorEditor::drawKnobPanel(juce::Graphics& g)
{
    auto panel = getPanelRect();
    int knobPanelW = 380;
    int knobPanelH = 160;
    float knobPanelBottom = panel.getBottom() - 30.0f; // pinned bottom position
    float knobPanelX = panel.getX() + (panel.getWidth() - knobPanelW) * 0.5f;
    float knobPanelY = knobPanelBottom - knobPanelH;

    // Dark panel background with rounded corners
    g.setColour(juce::Colour(0xFF0A0A14).withAlpha(0.92f));
    g.fillRoundedRectangle(knobPanelX, knobPanelY, (float)knobPanelW, (float)knobPanelH, 12.0f);

    // Subtle cyan border
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.25f));
    g.drawRoundedRectangle(knobPanelX, knobPanelY, (float)knobPanelW, (float)knobPanelH, 12.0f, 1.0f);

    // Subtle inner shadow / top highlight
    g.setColour(juce::Colours::white.withAlpha(0.03f));
    g.fillRoundedRectangle(knobPanelX + 1.0f, knobPanelY + 1.0f,
                           (float)knobPanelW - 2.0f, 20.0f, 12.0f);
}

//==============================================================================
// Output Level Meter
//==============================================================================
void OrionSoundEQAudioProcessorEditor::drawOutputMeter(juce::Graphics& g)
{
    auto panel = getPanelRect();

    // Meter bar fills the full gap between panel right edge and window right edge
    float meterX = panel.getRight() + 4.0f;
    float meterW = juce::jmax(6.0f, (float)getWidth() - meterX - 4.0f);
    float meterY = panel.getY();
    float meterH = panel.getHeight();

    // Dark background for meter
    g.setColour(juce::Colour(0xFF0A0A18).withAlpha(0.85f));
    g.fillRoundedRectangle(meterX - 1.0f, meterY, meterW + 2.0f, meterH, 2.0f);
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.3f));
    g.drawRoundedRectangle(meterX - 1.0f, meterY, meterW + 2.0f, meterH, 2.0f, 1.0f);

    // Split-scale mapping: 0dB at center, +24 at top, -∞ at bottom
    // Top half: linear +24 to 0 dB
    // Bottom half: compressed 0 to -∞ dB
    float midY = meterY + meterH * 0.5f;  // 0 dB line (center)
    float halfH = meterH * 0.5f;

    // Maps a dB value to a Y pixel position on the meter
    auto meterDbToY = [&](float db) -> float
    {
        if (db >= 0.0f)
        {
            // Top half: linear 0..+24 mapped to midY..meterY
            float t = juce::jlimit(0.0f, 1.0f, db / 24.0f);
            return midY - t * halfH;
        }
        else
        {
            // Bottom half: compressed, using log-ish curve so -∞ reaches bottom
            // Map 0 to 0, -48 to ~0.9, -∞ to 1.0
            float absDb = -db;
            float t = 1.0f - std::exp(-absDb / 18.0f); // exponential compression
            return midY + t * halfH;
        }
    };

    // Get current RMS level
    float rmsRaw = audioProcessor.getCurrentRMSLevel();
    float rmsDB = (rmsRaw > 0.0001f) ? 20.0f * std::log10(rmsRaw) : -100.0f;
    float fillTopY = meterDbToY(juce::jmin(rmsDB, 24.0f));
    float fillBotY = meterY + meterH; // fill from level down to bottom

    // Gradient: deep purple at bottom → cyan in mid → bright teal at top → warm for clipping
    juce::ColourGradient meterGrad(
        juce::Colour(0xFF0DCDD4), 0.0f, meterY,                     // top: bright cyan
        juce::Colour(0xFF2D1B69), 0.0f, meterY + meterH,            // bottom: deep purple
        false
    );
    meterGrad.addColour(0.05, juce::Colour(0xFFFF6B6B));             // clipping zone: warm red
    meterGrad.addColour(0.15, juce::Colour(0xFF0DCDD4));             // bright cyan
    meterGrad.addColour(0.5,  juce::Colour(0xFF0A8F96));             // 0dB center: mid teal
    meterGrad.addColour(0.85, juce::Colour(0xFF1A1060));             // dark purple-blue

    float fillH = fillBotY - fillTopY;
    if (fillH > 0.0f)
    {
        g.saveState();
        g.reduceClipRegion((int)meterX, (int)fillTopY, (int)meterW + 1, (int)fillH + 1);
        g.setGradientFill(meterGrad);
        g.fillRoundedRectangle(meterX, meterY, meterW, meterH, 1.5f);
        g.restoreState();
    }

    // Subtle glow when signal is present
    if (rmsDB > -80.0f)
    {
        float glowAlpha = juce::jlimit(0.0f, 0.25f, (rmsDB + 80.0f) / 300.0f);
        g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(glowAlpha));
        g.fillRoundedRectangle(meterX - 2.0f, fillTopY, meterW + 4.0f, fillH, 2.0f);
    }

    // dB scale labels inside the panel, just left of the right border
    g.setFont(juce::FontOptions(9.0f));

    // Top half labels: +24 to 0
    float topMarks[] = { 24.0f, 18.0f, 12.0f, 6.0f, 0.0f };
    for (float db : topMarks)
    {
        float yPos = meterDbToY(db);

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawHorizontalLine((int)yPos, panel.getRight() - 3.0f, panel.getRight());

        g.setColour(juce::Colours::white.withAlpha(0.6f));
        juce::String label = (db > 0 ? "+" : "") + juce::String((int)db);
        g.drawText(label, (int)(panel.getRight() - 30.0f), (int)(yPos - 5), 26, 10,
                   juce::Justification::centredRight, false);
    }

    // Bottom half labels: -6 to -48, then ∞
    float botMarks[] = { -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f };
    for (float db : botMarks)
    {
        float yPos = meterDbToY(db);

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawHorizontalLine((int)yPos, panel.getRight() - 3.0f, panel.getRight());

        g.setColour(juce::Colours::white.withAlpha(0.6f));
        juce::String label = juce::String((int)db);
        g.drawText(label, (int)(panel.getRight() - 30.0f), (int)(yPos - 5), 26, 10,
                   juce::Justification::centredRight, false);
    }

    // Infinity label at the very bottom
    {
        float yPos = meterY + meterH - 2.0f;
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawHorizontalLine((int)yPos, panel.getRight() - 3.0f, panel.getRight());
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawText("-" + juce::String::charToString((juce::juce_wchar)0x221E),
                   (int)(panel.getRight() - 30.0f), (int)(yPos - 5), 26, 10,
                   juce::Justification::centredRight, false);
    }

    // === Draggable blue output gain fill bar ===
    {
        float gainY = meterDbToY(outputGainDB);
        float fillBot = meterY + meterH;
        float barFillH = fillBot - gainY;

        if (barFillH > 0.0f)
        {
            juce::Colour barCol = juce::Colour(0xFF0DCDD4); // cyan/teal matching bottom
            if (draggingOutputGain)
                barCol = barCol.brighter(0.3f);

            // Blue fill from bottom up to the gain level
            juce::ColourGradient barGrad(
                barCol.withAlpha(0.85f), 0.0f, gainY,
                barCol.darker(0.4f).withAlpha(0.6f), 0.0f, fillBot,
                false
            );
            g.setGradientFill(barGrad);
            g.fillRect(meterX + 1.0f, gainY, meterW - 2.0f, barFillH);

            // Bright top edge line
            g.setColour(barCol.brighter(0.4f).withAlpha(0.95f));
            g.fillRect(meterX + 1.0f, gainY, meterW - 2.0f, 2.0f);

            // Subtle glow at the top edge
            g.setColour(barCol.withAlpha(0.3f));
            g.fillRect(meterX - 1.0f, gainY - 2.0f, meterW + 2.0f, 6.0f);
        }
    }
}

//==============================================================================
// Page visibility control
//==============================================================================
void OrionSoundEQAudioProcessorEditor::setPageVisibility(bool mainPage)
{
    // Main page controls
    bandCountCombo.setVisible(mainPage);
    bandModeCombo.setVisible(mainPage);
    filterCombo.setVisible(mainPage);
    powerButton.setVisible(mainPage);
    presetCombo.setVisible(mainPage);
    savePresetButton.setVisible(mainPage);
    undoButton.setVisible(mainPage);
    redoButton.setVisible(mainPage);
    freqKnob.setVisible(mainPage);
    gainKnob.setVisible(mainPage);
    qKnob.setVisible(mainPage);
    freqKnobLabel.setVisible(mainPage);
    gainKnobLabel.setVisible(mainPage);
    qKnobLabel.setVisible(mainPage);

    // Bottom bar controls — visible on main page
    phaseModeCombo.setVisible(mainPage);
    spectrumButton.setVisible(mainPage);
    midiLearnButton.setVisible(mainPage);
}

//==============================================================================
// Settings Page
//==============================================================================
void OrionSoundEQAudioProcessorEditor::drawSettingsPage(juce::Graphics& g)
{
    auto panel = getPanelRect();

    // Dark settings panel background
    g.setColour(juce::Colour(0xFF0A0A18).withAlpha(0.85f));
    g.fillRoundedRectangle(panel, 8.0f);

    // Panel border
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.25f));
    g.drawRoundedRectangle(panel, 8.0f, 1.0f);

    // Settings title
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::FontOptions(22.0f));
    g.drawText("Settings", panel.getX(), panel.getY() + 20.0f, panel.getWidth(), 30,
               juce::Justification::centred, false);

    // Divider line below title
    float dividerY = panel.getY() + 58.0f;
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.15f));
    g.drawLine(panel.getX() + 30.0f, dividerY, panel.getRight() - 30.0f, dividerY, 1.0f);

    // Vertical divider splitting the page in half
    float midX = panel.getCentreX();
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.15f));
    g.drawLine(midX, dividerY + 10.0f, midX, panel.getBottom() - 40.0f, 1.0f);

    float rowH = 28.0f;
    float startY = dividerY + 25.0f;
    float leftCol = panel.getX() + 30.0f;
    float leftW = midX - leftCol - 20.0f;
    float rightCol = midX + 20.0f;
    float rightW = panel.getRight() - rightCol - 30.0f;

    // ============ LEFT SIDE: Display & Audio ============

    // --- Display section ---
    g.setFont(juce::FontOptions(15.0f));
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.9f));
    g.drawText("Display", leftCol, startY, leftW, 20, juce::Justification::centredLeft, false);

    g.setFont(juce::FontOptions(12.0f));

    juce::String displaySettings[] = {
        "Aurora Animation: On",
        "Grid Opacity: 100%",
        "Curve Thickness: Normal",
        "Handle Size: Normal",
        "Theme: Space"
    };

    for (int i = 0; i < 5; ++i)
    {
        float y = startY + 28.0f + i * rowH;
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawText(displaySettings[i], leftCol + 10.0f, y, leftW, 20,
                   juce::Justification::centredLeft, false);
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.drawLine(leftCol + 10.0f, y + 22.0f, leftCol + leftW, y + 22.0f, 0.5f);
    }

    // --- Audio section ---
    float audioStartY = startY + 28.0f + 5 * rowH + 20.0f;

    // Section divider
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.1f));
    g.drawLine(leftCol + 10.0f, audioStartY - 10.0f, leftCol + leftW, audioStartY - 10.0f, 0.5f);

    g.setFont(juce::FontOptions(15.0f));
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.9f));
    g.drawText("Audio", leftCol, audioStartY, leftW, 20, juce::Justification::centredLeft, false);

    g.setFont(juce::FontOptions(12.0f));

    juce::String audioSettings[] = {
        "Sample Rate: 44100 Hz",
        "Buffer Size: 512",
        "Latency: 11.6 ms",
        "Oversampling: Off",
        "Channels: Stereo"
    };

    for (int i = 0; i < 5; ++i)
    {
        float y = audioStartY + 28.0f + i * rowH;
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawText(audioSettings[i], leftCol + 10.0f, y, leftW, 20,
                   juce::Justification::centredLeft, false);
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.drawLine(leftCol + 10.0f, y + 22.0f, leftCol + leftW, y + 22.0f, 0.5f);
    }

    // ============ RIGHT SIDE: Key Binds ============

    g.setFont(juce::FontOptions(15.0f));
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.9f));
    g.drawText("Key Binds", rightCol, startY, rightW, 20, juce::Justification::centredLeft, false);

    g.setFont(juce::FontOptions(12.0f));

    // Hint text
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawText("(Click to edit)", rightCol, startY + 2.0f, rightW, 16,
               juce::Justification::centredRight, false);

    struct KeyBind { juce::String action; juce::String key; bool editable; };
    KeyBind keyBinds[] = {
        { "Delete Point",      "Backspace", true },
        { "Add Point",         "Cmd/Ctrl + Click", false },
        { "Undo",              "Cmd + Z", true },
        { "Redo",              "Cmd + Shift + Z", true },
    };

    for (int i = 0; i < 4; ++i)
    {
        float y = startY + 28.0f + i * rowH;

        // Action name
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawText(keyBinds[i].action, rightCol + 10.0f, y, rightW * 0.5f, 20,
                   juce::Justification::centredLeft, false);

        // Key binding (right-aligned, styled)
        float keyX = rightCol + rightW * 0.5f;
        float keyW = rightW * 0.45f;

        // Highlight if editing
        if (editingKeybindIndex == i)
            g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.2f));
        else
            g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.fillRoundedRectangle(keyX, y, keyW, 20.0f, 3.0f);

        // Border
        if (editingKeybindIndex == i)
            g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.5f));
        else
            g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(keyX, y, keyW, 20.0f, 3.0f, 0.5f);

        // Key text or "Press key..."
        g.setFont(juce::FontOptions(12.0f));
        if (editingKeybindIndex == i)
        {
            g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.9f));
            g.drawText("Press key...", keyX, y, keyW, 20,
                       juce::Justification::centred, false);
        }
        else
        {
            g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.7f));
            g.drawText(keyBinds[i].key, keyX, y, keyW, 20,
                       juce::Justification::centred, false);
        }

        // Row divider
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.drawLine(rightCol + 10.0f, y + 22.0f, rightCol + rightW, y + 22.0f, 0.5f);
    }

    // Reset button
    float resetY = panel.getBottom() - 60.0f;
    float resetW = 120.0f;
    float resetX = midX - resetW * 0.5f;

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(resetX, resetY, resetW, 28.0f, 4.0f);
    g.setColour(juce::Colour(0xFF0DCDD4).withAlpha(0.3f));
    g.drawRoundedRectangle(resetX, resetY, resetW, 28.0f, 4.0f, 1.0f);

    g.setFont(juce::FontOptions(13.0f));
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawText("Reset Keybinds", (int)resetX, (int)resetY, (int)resetW, 28,
               juce::Justification::centred, false);

    // Version info at the bottom
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawText("Orion Sound EQ v1.0.0", panel.getX(), panel.getBottom() - 30.0f,
               panel.getWidth(), 20, juce::Justification::centred, false);
}
