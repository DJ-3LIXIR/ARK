#pragma once

#include <JuceHeader.h>
// BinaryData included via JuceHeader (ARK's resources — room images fall back to disk loading)

class APOLLOAudioProcessor;

class RoomSimPage : public juce::Component
{
public:
    // --- Small toggle button with custom icon painting ---
    class SmallToggleButton : public juce::TextButton
    {
    public:
        enum IconType { Power, Phase, Mono, Lock };
        IconType iconType = Power;

        juce::Colour onColour  { juce::Colour::fromRGB (230, 176, 46) };
        juce::Colour offColour { juce::Colours::white.withAlpha (0.25f) };

        SmallToggleButton() { setClickingTogglesState (true); }

        void paintButton (juce::Graphics& g, bool isHighlighted, bool /*isDown*/) override
        {
            auto b = getLocalBounds().toFloat().reduced (2.0f);
            auto c = b.getCentre();
            bool on = getToggleState();
            auto col = on ? onColour : offColour;
            if (isHighlighted) col = col.brighter (0.2f);

            // Subtle background
            g.setColour (juce::Colour::fromRGB (38, 40, 46));
            g.fillRoundedRectangle (b, 3.0f);
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.drawRoundedRectangle (b, 3.0f, 1.0f);

            g.setColour (col);

            switch (iconType)
            {
                case Power:
                {
                    float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.28f;
                    juce::Path arc;
                    float gap = 0.65f;
                    arc.addCentredArc (c.x, c.y, r, r, 0.0f,
                                       -juce::MathConstants<float>::halfPi + gap,
                                       juce::MathConstants<float>::pi * 1.5f - gap, true);
                    g.strokePath (arc, juce::PathStrokeType (1.4f));
                    g.drawLine (c.x, c.y, c.x, c.y - r * 1.1f, 1.4f);
                    break;
                }
                case Phase:
                {
                    g.setFont (juce::FontOptions (13.0f));
                    g.drawText (juce::String::charToString (0x00D8), b,
                                juce::Justification::centred, false);
                    break;
                }
                case Mono:
                {
                    g.setFont (juce::FontOptions (11.0f));
                    g.drawText ("M", b, juce::Justification::centred, false);
                    break;
                }
                case Lock:
                {
                    g.setFont (juce::FontOptions (11.0f));
                    g.drawText ("LK", b, juce::Justification::centred, false);
                    break;
                }
            }
        }
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    RoomSimPage (APOLLOAudioProcessor& p, juce::AudioProcessorValueTreeState& vts) : audioProcessor (p), apvts (vts), currentPresetIndex (0)
    {
        // =================================================================
        // Preset strip components
        // =================================================================
        roomSimPowerBtn.iconType = SmallToggleButton::Power;
        roomSimPowerBtn.setToggleState (true, juce::dontSendNotification);
        addAndMakeVisible (roomSimPowerBtn);

        presetCategoryButton.setButtonText ("Factory Presets");
        presetCategoryButton.setColour (juce::TextButton::buttonColourId, presetBtnColour);
        presetCategoryButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.85f));
        addAndMakeVisible (presetCategoryButton);

        presetNameLabel.setText ("Default Room", juce::dontSendNotification);
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
        savePresetButton.setColour (juce::TextButton::textColourOffId, goldAccent);
        addAndMakeVisible (savePresetButton);

        // --- Preset button handlers ---
        prevPresetButton.onClick = [this] { cyclePreset (-1); };
        nextPresetButton.onClick = [this] { cyclePreset (1); };
        presetCategoryButton.onClick = [this] { showPresetMenu(); };

        // Load initial preset (savedPresetIndex[1] encodes roomType * 5 + subPreset)
        {
            int savedIdx = juce::jlimit (0, numRoomTypes * numSubPresets - 1, audioProcessor.savedPresetIndex[1]);
            currentRoomType = savedIdx / numSubPresets;
            currentPresetIndex = savedIdx % numSubPresets;
            roomButtons[currentRoomType].setToggleState (true, juce::dontSendNotification);
        }
        updatePresetName();
        loadRoomImage (currentRoomType);

        // =================================================================
        // Room type buttons
        // =================================================================
        const juce::String names[] = { "Theater", "Church", "Studio", "Hall",
                                        "Garage", "Bathroom", "Bedroom", "Warehouse" };

        for (int i = 0; i < numRoomTypes; ++i)
        {
            roomButtons[i].setButtonText (names[i]);
            roomButtons[i].setClickingTogglesState (true);
            roomButtons[i].setRadioGroupId (1001);
            roomButtons[i].setColour (juce::TextButton::buttonColourId, roomBtnInactive);
            roomButtons[i].setColour (juce::TextButton::buttonOnColourId, roomBtnActive);
            roomButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.55f));
            roomButtons[i].setColour (juce::TextButton::textColourOnId, goldAccent);
            addAndMakeVisible (roomButtons[i]);
        }
        roomButtons[0].setToggleState (true, juce::dontSendNotification);

        // =================================================================
        // Input & Output knobs (top half — flanking room image)
        // =================================================================
        setupKnob (inputKnob, inputLabel, "Input");
        setupKnob (outputKnob, outputLabel, "Output");

        inputKnob.setRange (0.0, 100.0, 0.1);
        inputKnob.setValue (100.0);
        inputKnob.setTextValueSuffix (" %");

        outputKnob.setRange (0.0, 100.0, 0.1);
        outputKnob.setValue (100.0);
        outputKnob.setTextValueSuffix (" %");

        // =================================================================
        // Bottom half — Room parameter knobs (Section 1: left)
        // =================================================================
        setupKnob (roomSizeKnob,    roomSizeLabel,    "Room Size");
        setupKnob (absorptionKnob,  absorptionLabel,  "Absorption");
        setupKnob (diffusionKnob,   diffusionLabel,   "Diffusion");

        roomSizeKnob.setRange (1.0, 100.0, 0.1);
        roomSizeKnob.setValue (40.0);
        roomSizeKnob.setTextValueSuffix (" m");

        absorptionKnob.setRange (0.0, 100.0, 0.1);
        absorptionKnob.setValue (50.0);
        absorptionKnob.setTextValueSuffix (" %");

        diffusionKnob.setRange (0.0, 100.0, 0.1);
        diffusionKnob.setValue (65.0);
        diffusionKnob.setTextValueSuffix (" %");

        // =================================================================
        // Bottom half — Timing knobs (Section 2: center-left)
        // =================================================================
        setupKnob (predelayKnob, predelayLabel, "Pre-Delay");
        setupKnob (decayKnob,   decayLabel,   "Decay");

        predelayKnob.setRange (0.0, 200.0, 1.0);
        predelayKnob.setValue (12.0);
        predelayKnob.setTextValueSuffix (" ms");

        decayKnob.setRange (0.1, 30.0, 0.01);
        decayKnob.setValue (2.0);
        decayKnob.setTextValueSuffix (" s");
        decayKnob.setSkewFactorFromMidPoint (3.0);

        // =================================================================
        // Bottom half — Spatial knobs (Section 3: center-right)
        // =================================================================
        setupKnob (widthKnob,     widthLabel,     "Width");
        setupKnob (earlyLateKnob, earlyLateLabel, "Early/Late");

        widthKnob.setRange (0.0, 200.0, 0.1);
        widthKnob.setValue (100.0);
        widthKnob.setTextValueSuffix (" %");

        earlyLateKnob.setRange (0.0, 100.0, 0.1);
        earlyLateKnob.setValue (50.0);
        earlyLateKnob.setTextValueSuffix (" %");

        // =================================================================
        // Bottom half — Dry / Wet faders (Section 4: right)
        // =================================================================
        setupFader (dryFader, dryLabel, "Dry");
        setupFader (wetFader, wetLabel, "Wet");

        dryFader.setRange (0.0, 100.0, 0.1);
        dryFader.setValue (100.0);
        dryFader.setTextValueSuffix (" %");

        wetFader.setRange (0.0, 100.0, 0.1);
        wetFader.setValue (50.0);
        wetFader.setTextValueSuffix (" %");

        // =================================================================
        // Bottom half — Toggle buttons (Section 4: right, beside faders)
        // =================================================================
        bypassBtn.iconType = SmallToggleButton::Power;
        phaseBtn.iconType  = SmallToggleButton::Phase;
        monoBtn.iconType   = SmallToggleButton::Mono;
        lockBtn.iconType   = SmallToggleButton::Lock;

        addAndMakeVisible (bypassBtn);
        addAndMakeVisible (phaseBtn);
        addAndMakeVisible (monoBtn);
        addAndMakeVisible (lockBtn);

        // =================================================================
        // APVTS Attachments
        // =================================================================
        inputGainAttach    = std::make_unique<SliderAttachment> (apvts, "rsInputGain",   inputKnob);
        outputGainAttach   = std::make_unique<SliderAttachment> (apvts, "rsOutputGain",  outputKnob);
        roomSizeAttach     = std::make_unique<SliderAttachment> (apvts, "rsRoomSize",    roomSizeKnob);
        absorptionAttach   = std::make_unique<SliderAttachment> (apvts, "rsAbsorption",  absorptionKnob);
        diffusionAttach    = std::make_unique<SliderAttachment> (apvts, "rsDiffusion",   diffusionKnob);
        predelayAttach     = std::make_unique<SliderAttachment> (apvts, "rsPredelay",    predelayKnob);
        decayAttach        = std::make_unique<SliderAttachment> (apvts, "rsDecay",       decayKnob);
        widthAttach        = std::make_unique<SliderAttachment> (apvts, "rsWidth",       widthKnob);
        earlyLateAttach    = std::make_unique<SliderAttachment> (apvts, "rsEarlyLate",   earlyLateKnob);
        dryAttach          = std::make_unique<SliderAttachment> (apvts, "rsDry",         dryFader);
        wetAttach          = std::make_unique<SliderAttachment> (apvts, "rsWet",         wetFader);

        powerAttach        = std::make_unique<ButtonAttachment> (apvts, "rsOn",    roomSimPowerBtn);
        bypassAttach       = std::make_unique<ButtonAttachment> (apvts, "rsOn",    bypassBtn);
        phaseAttach        = std::make_unique<ButtonAttachment> (apvts, "rsPhase", phaseBtn);
        monoAttach         = std::make_unique<ButtonAttachment> (apvts, "rsMono",  monoBtn);
        lockAttach         = std::make_unique<ButtonAttachment> (apvts, "rsLock",  lockBtn);

        // Wire room type radio buttons to rsRoomType choice parameter
        for (int i = 0; i < numRoomTypes; ++i)
        {
            roomButtons[i].onClick = [this, i]
            {
                currentRoomType = i;
                currentPresetIndex = 0;
                audioProcessor.loadRoomSimPreset (currentRoomType, currentPresetIndex);
                audioProcessor.savedPresetIndex[1] = currentRoomType * numSubPresets + currentPresetIndex;
                updatePresetName();
                loadRoomImage (currentRoomType);
                if (auto* param = apvts.getParameter ("rsRoomType"))
                    param->setValueNotifyingHost (param->convertTo0to1 ((float) i));
            };
        }

        // Listen for external changes to rsRoomType
        apvts.addParameterListener ("rsRoomType", &roomTypeListener);
    }

    ~RoomSimPage()
    {
        apvts.removeParameterListener ("rsRoomType", &roomTypeListener);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // =================================================================
        // Preset strip background
        // =================================================================
        g.setColour (presetStripBg);
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), presetStripH);

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawHorizontalLine (bounds.getY() + presetStripH - 1,
                              (float) bounds.getX(), (float) bounds.getRight());
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.drawHorizontalLine (bounds.getY() + presetStripH,
                              (float) bounds.getX(), (float) bounds.getRight());

        // =================================================================
        // Room visualization area (dark background — top half)
        // =================================================================
        int roomAreaTop = bounds.getY() + presetStripH;
        int remainH = bounds.getHeight() - presetStripH;
        int roomAreaH = juce::roundToInt (remainH * topProportion);
        auto roomArea = juce::Rectangle<int> (bounds.getX(), roomAreaTop,
                                               bounds.getWidth(), roomAreaH);
        g.setColour (gridBackground);
        g.fillRect (roomArea);

        // --- Room image area ---
        auto imageArea = roomArea.withTrimmedLeft (knobColumnW)
                                 .withTrimmedRight (knobColumnW)
                                 .withTrimmedBottom (roomBtnStripH);
        auto imageBounds = imageArea.reduced (16, 16);

        if (!currentRoomImage.isNull())
        {
            // Draw the room image scaled to fit
            g.drawImageWithin (currentRoomImage, imageBounds.getX(), imageBounds.getY(),
                              imageBounds.getWidth(), imageBounds.getHeight(),
                              juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                              false);
        }
        else
        {
            // Placeholder when no image is loaded
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.fillRoundedRectangle (imageBounds.toFloat(), 6.0f);

            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.drawRoundedRectangle (imageBounds.toFloat(), 6.0f, 1.0f);

            g.setColour (juce::Colours::white.withAlpha (0.2f));
            g.setFont (juce::FontOptions (16.0f));
            g.drawFittedText ("Room Image", imageBounds,
                              juce::Justification::centred, 2);
        }

        // --- Room button strip background ---
        auto btnStripBg = juce::Rectangle<int> (
            roomArea.getX() + knobColumnW,
            roomArea.getBottom() - roomBtnStripH,
            roomArea.getWidth() - 2 * knobColumnW,
            roomBtnStripH);

        g.setColour (juce::Colour::fromRGB (22, 24, 30));
        g.fillRect (btnStripBg);

        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine (btnStripBg.getY(),
                              (float) btnStripBg.getX(), (float) btnStripBg.getRight());

        // --- Subtle labels for Input / Output columns ---
        {
            auto leftCol = roomArea.withWidth (knobColumnW);
            auto rightCol = roomArea.withLeft (roomArea.getRight() - knobColumnW);

            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.setFont (juce::FontOptions (10.0f));
            g.drawText ("IN", leftCol.removeFromTop (28), juce::Justification::centred, false);
            g.drawText ("OUT", rightCol.removeFromTop (28), juce::Justification::centred, false);
        }

        // =================================================================
        // Bottom half: silver control panel
        // =================================================================
        int controlAreaTop = roomArea.getBottom();
        int controlAreaH = bounds.getBottom() - controlAreaTop;
        auto controlArea = juce::Rectangle<int> (bounds.getX(), controlAreaTop,
                                                  bounds.getWidth(), controlAreaH);

        juce::ColourGradient silverGrad (silverTop, 0.0f, (float) controlArea.getY(),
                                         silverBottom, 0.0f, (float) controlArea.getBottom(), false);
        g.setGradientFill (silverGrad);
        g.fillRect (controlArea);

        // Top divider
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawHorizontalLine (controlArea.getY(),
                              (float) controlArea.getX(), (float) controlArea.getRight());
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawHorizontalLine (controlArea.getY() + 1,
                              (float) controlArea.getX(), (float) controlArea.getRight());

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

        int s1W = juce::roundToInt (totalW * 0.34f);
        int s2W = juce::roundToInt (totalW * 0.18f);
        int s3W = juce::roundToInt (totalW * 0.22f);

        drawSeparator (ctrlInner.getX() + s1W);
        drawSeparator (ctrlInner.getX() + s1W + s2W);
        drawSeparator (ctrlInner.getX() + s1W + s2W + s3W);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // =================================================================
        // Preset strip layout
        // =================================================================
        auto presetArea = juce::Rectangle<int> (bounds.getX(), bounds.getY(),
                                                 bounds.getWidth(), presetStripH)
                              .reduced (12, 5);
        int presetBtnH = presetArea.getHeight();

        roomSimPowerBtn.setBounds (presetArea.removeFromLeft (presetBtnH).withHeight (presetBtnH));
        presetArea.removeFromLeft (6);
        presetCategoryButton.setBounds (presetArea.removeFromLeft (130).withHeight (presetBtnH));
        presetArea.removeFromLeft (8);

        savePresetButton.setBounds (presetArea.removeFromRight (50).withHeight (presetBtnH));
        presetArea.removeFromRight (6);
        nextPresetButton.setBounds (presetArea.removeFromRight (28).withHeight (presetBtnH));
        presetArea.removeFromRight (2);
        prevPresetButton.setBounds (presetArea.removeFromRight (28).withHeight (presetBtnH));
        presetArea.removeFromRight (8);

        presetNameLabel.setBounds (presetArea.withHeight (presetBtnH));

        // =================================================================
        // Room visualization area
        // =================================================================
        int remainH = bounds.getHeight() - presetStripH;
        int roomAreaH = juce::roundToInt (remainH * topProportion);
        auto roomArea = juce::Rectangle<int> (bounds.getX(), bounds.getY() + presetStripH,
                                               bounds.getWidth(), roomAreaH);

        // --- Input knob (left column) ---
        {
            auto leftCol = roomArea.withWidth (knobColumnW);
            int kw = 90, kh = 80, lh = 18;
            int totalH = lh + kh;
            int y = leftCol.getCentreY() - totalH / 2;
            int x = leftCol.getCentreX() - kw / 2;
            inputLabel.setBounds (x, y, kw, lh);
            inputKnob.setBounds (x, y + lh, kw, kh);
        }

        // --- Output knob (right column) ---
        {
            auto rightCol = roomArea.withLeft (roomArea.getRight() - knobColumnW);
            int kw = 90, kh = 80, lh = 18;
            int totalH = lh + kh;
            int y = rightCol.getCentreY() - totalH / 2;
            int x = rightCol.getCentreX() - kw / 2;
            outputLabel.setBounds (x, y, kw, lh);
            outputKnob.setBounds (x, y + lh, kw, kh);
        }

        // --- Room type buttons ---
        {
            auto btnStrip = juce::Rectangle<int> (
                roomArea.getX() + knobColumnW + 12,
                roomArea.getBottom() - roomBtnStripH + 7,
                roomArea.getWidth() - 2 * knobColumnW - 24,
                roomBtnStripH - 14);

            int btnGap = 6;
            int totalGaps = (numRoomTypes - 1) * btnGap;
            int btnW = (btnStrip.getWidth() - totalGaps) / numRoomTypes;

            for (int i = 0; i < numRoomTypes; ++i)
            {
                roomButtons[i].setBounds (btnStrip.getX() + i * (btnW + btnGap),
                                          btnStrip.getY(),
                                          btnW,
                                          btnStrip.getHeight());
            }
        }

        // =================================================================
        // Bottom half — Control panel layout
        // =================================================================
        auto controlArea = juce::Rectangle<int> (
            bounds.getX(), roomArea.getBottom(),
            bounds.getWidth(), bounds.getBottom() - roomArea.getBottom())
                .reduced (20, 15);

        int knobW  = 105;
        int knobH  = 95;
        int labelH = 18;
        int faderW = 55;

        int totalW = controlArea.getWidth();
        int rowH   = labelH + knobH;

        int sec1W = juce::roundToInt (totalW * 0.34f);
        int sec2W = juce::roundToInt (totalW * 0.18f);
        int sec3W = juce::roundToInt (totalW * 0.22f);
        int sec4W = totalW - sec1W - sec2W - sec3W;

        int sec1X = controlArea.getX();
        int sec2X = sec1X + sec1W;
        int sec3X = sec2X + sec2W;
        int sec4X = sec3X + sec3W;

        int rowY = controlArea.getY() + (controlArea.getHeight() - rowH) / 2 - 8;

        // --- Section 1: Room Size, Absorption, Diffusion ---
        {
            int groupW = 3 * knobW;
            int gap = (sec1W - groupW) / 4;
            int x = sec1X + gap;
            layoutKnob (roomSizeLabel,   roomSizeKnob,   x, rowY, knobW, labelH, knobH);
            x += knobW + gap;
            layoutKnob (absorptionLabel, absorptionKnob, x, rowY, knobW, labelH, knobH);
            x += knobW + gap;
            layoutKnob (diffusionLabel,  diffusionKnob,  x, rowY, knobW, labelH, knobH);
        }

        // --- Section 2: Pre-Delay, Decay ---
        {
            int groupW = 2 * knobW;
            int gap = (sec2W - groupW) / 3;
            int x = sec2X + gap;
            layoutKnob (predelayLabel, predelayKnob, x, rowY, knobW, labelH, knobH);
            x += knobW + gap;
            layoutKnob (decayLabel,    decayKnob,    x, rowY, knobW, labelH, knobH);
        }

        // --- Section 3: Width, Early/Late ---
        {
            int groupW = 2 * knobW;
            int gap = (sec3W - groupW) / 3;
            int x = sec3X + gap;
            layoutKnob (widthLabel,     widthKnob,     x, rowY, knobW, labelH, knobH);
            x += knobW + gap;
            layoutKnob (earlyLateLabel, earlyLateKnob, x, rowY, knobW, labelH, knobH);
        }

        // --- Section 4: Dry/Wet faders + toggle buttons ---
        {
            int faderH = rowH + 30;
            int faderGap = 8;
            int toggleBtnSize = 26;
            int toggleGap = 5;

            // Faders on the left of this section
            int fadersW = 2 * faderW + faderGap;
            // Toggle column on the right
            int toggleColW = toggleBtnSize;
            int totalNeeded = fadersW + 18 + toggleColW;
            int sectionPad = (sec4W - totalNeeded) / 2;

            int fx = sec4X + sectionPad;
            layoutFader (dryLabel, dryFader, fx, rowY, faderW, labelH, faderH);
            fx += faderW + faderGap;
            layoutFader (wetLabel, wetFader, fx, rowY, faderW, labelH, faderH);

            // Toggle buttons stacked vertically to the right of faders
            int tx = fx + faderW + 18;
            int ty = rowY + labelH + 4;
            bypassBtn.setBounds (tx, ty, toggleBtnSize, toggleBtnSize);
            ty += toggleBtnSize + toggleGap;
            phaseBtn.setBounds (tx, ty, toggleBtnSize, toggleBtnSize);
            ty += toggleBtnSize + toggleGap;
            monoBtn.setBounds (tx, ty, toggleBtnSize, toggleBtnSize);
            ty += toggleBtnSize + toggleGap;
            lockBtn.setBounds (tx, ty, toggleBtnSize, toggleBtnSize);
        }
    }

private:
    // =====================================================================
    // Helpers
    // =====================================================================
    void setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& name)
    {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
        knob.setColour (juce::Slider::rotarySliderFillColourId, goldAccent);
        knob.setColour (juce::Slider::rotarySliderOutlineColourId, knobTrack);
        knob.setColour (juce::Slider::thumbColourId, goldAccent);
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
        fader.setColour (juce::Slider::trackColourId, goldAccent);
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

    void cyclePreset (int direction)
    {
        currentPresetIndex += direction;
        if (currentPresetIndex < 0) currentPresetIndex = numSubPresets - 1;
        if (currentPresetIndex >= numSubPresets) currentPresetIndex = 0;

        audioProcessor.loadRoomSimPreset (currentRoomType, currentPresetIndex);
        audioProcessor.savedPresetIndex[1] = currentRoomType * numSubPresets + currentPresetIndex;
        updatePresetName();
    }

    void updatePresetName()
    {
        if (currentPresetIndex >= 0 && currentPresetIndex < subPresetNames.size()
            && currentRoomType >= 0 && currentRoomType < numRoomTypes)
        {
            const juce::String roomNames[] = { "Theater", "Church", "Studio", "Hall",
                                                "Garage", "Bathroom", "Bedroom", "Warehouse" };
            presetNameLabel.setText (roomNames[currentRoomType] + " - " + subPresetNames[currentPresetIndex],
                                    juce::dontSendNotification);
        }
    }

    void loadRoomImage (int roomType)
    {
        if (roomType >= 0 && roomType < roomImageNames.size())
        {
            // Load image from embedded resources
            // Projucer converts "Theater.png" to "Theater_png" in BinaryData
            juce::String resourceName = roomImageNames[roomType] + "_png";
            int resourceSize = 0;
            const char* resourceData = BinaryData::getNamedResource (resourceName.toRawUTF8(), resourceSize);

            if (resourceData != nullptr && resourceSize > 0)
            {
                currentRoomImage = juce::ImageFileFormat::loadFrom (resourceData, (size_t)resourceSize);
            }
            else
            {
                // Fallback: try loading from disk if embedded resource fails
                juce::Array<juce::File> searchPaths;
                searchPaths.add (juce::File::getSpecialLocation (juce::File::SpecialLocationType::userHomeDirectory)
                                    .getChildFile ("Synth Projects")
                                    .getChildFile ("APOLLO")
                                    .getChildFile ("APOLLO")
                                    .getChildFile ("Assets")
                                    .getChildFile ("RoomImages"));
                searchPaths.add (juce::File::getCurrentWorkingDirectory()
                                   .getChildFile ("Assets")
                                   .getChildFile ("RoomImages"));

                juce::File imageFile;
                for (const auto& searchPath : searchPaths)
                {
                    juce::File candidate = searchPath.getChildFile (roomImageNames[roomType] + ".png");
                    if (candidate.exists())
                    {
                        imageFile = candidate;
                        break;
                    }
                }

                if (imageFile.exists())
                {
                    currentRoomImage = juce::ImageFileFormat::loadFrom (imageFile);
                }
                else
                {
                    currentRoomImage = juce::Image();
                }
            }

            repaint();
        }
    }

    void showPresetMenu()
    {
        const juce::String roomNames[] = { "Theater", "Church", "Studio", "Hall",
                                            "Garage", "Bathroom", "Bedroom", "Warehouse" };
        juce::PopupMenu menu;

        for (int r = 0; r < numRoomTypes; ++r)
        {
            juce::PopupMenu subMenu;
            for (int s = 0; s < numSubPresets; ++s)
            {
                int itemId = r * numSubPresets + s + 1;
                bool isCurrent = (r == currentRoomType && s == currentPresetIndex);
                subMenu.addItem (itemId, subPresetNames[s], true, isCurrent);
            }
            menu.addSubMenu (roomNames[r], subMenu, true, juce::Image(), currentRoomType == r);
        }

        menu.showMenuAsync (juce::PopupMenu::Options(),
            [this] (int result) {
                if (result > 0) {
                    int idx = result - 1;
                    currentRoomType = idx / numSubPresets;
                    currentPresetIndex = idx % numSubPresets;
                    audioProcessor.loadRoomSimPreset (currentRoomType, currentPresetIndex);
                    audioProcessor.savedPresetIndex[1] = idx;
                    roomButtons[currentRoomType].setToggleState (true, juce::dontSendNotification);
                    updatePresetName();
                    loadRoomImage (currentRoomType);
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

    // =====================================================================
    // Top half components
    // =====================================================================

    // Room sim power button
    SmallToggleButton roomSimPowerBtn;

    // Preset strip
    juce::TextButton presetCategoryButton;
    juce::Label      presetNameLabel;
    juce::TextButton prevPresetButton;
    juce::TextButton nextPresetButton;
    juce::TextButton savePresetButton;

    // Room type buttons
    static constexpr int numRoomTypes = 8;
    juce::TextButton roomButtons[numRoomTypes];

    // Input / Output knobs (flanking room image)
    juce::Slider inputKnob, outputKnob;
    juce::Label  inputLabel, outputLabel;

    // =====================================================================
    // Bottom half components
    // =====================================================================

    // Section 1: Room parameters
    juce::Slider roomSizeKnob, absorptionKnob, diffusionKnob;
    juce::Label  roomSizeLabel, absorptionLabel, diffusionLabel;

    // Section 2: Timing
    juce::Slider predelayKnob, decayKnob;
    juce::Label  predelayLabel, decayLabel;

    // Section 3: Spatial
    juce::Slider widthKnob, earlyLateKnob;
    juce::Label  widthLabel, earlyLateLabel;

    // Section 4: Output
    juce::Slider dryFader, wetFader;
    juce::Label  dryLabel, wetLabel;

    // Toggle buttons
    SmallToggleButton bypassBtn, phaseBtn, monoBtn, lockBtn;

    // =====================================================================
    // APVTS reference & attachments
    // =====================================================================
    APOLLOAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;

    // Preset management
    int currentPresetIndex = 0;
    int currentRoomType = 0;
    static constexpr int numSubPresets = 5;
    const juce::StringArray subPresetNames { "Bright", "Warm", "Dark", "Ambient", "Tight" };

    // Room images
    juce::Image currentRoomImage;
    const juce::StringArray roomImageNames { "Theater", "Church", "Studio", "Hall",
                                              "Garage", "Bathroom", "Bedroom", "Warehouse" };

    std::unique_ptr<SliderAttachment> inputGainAttach, outputGainAttach;
    std::unique_ptr<SliderAttachment> roomSizeAttach, absorptionAttach, diffusionAttach;
    std::unique_ptr<SliderAttachment> predelayAttach, decayAttach;
    std::unique_ptr<SliderAttachment> widthAttach, earlyLateAttach;
    std::unique_ptr<SliderAttachment> dryAttach, wetAttach;

    std::unique_ptr<ButtonAttachment> powerAttach, bypassAttach, phaseAttach, monoAttach, lockAttach;

    // Listener for room type changes from host automation
    struct RoomTypeListener : juce::AudioProcessorValueTreeState::Listener
    {
        RoomSimPage* owner = nullptr;
        explicit RoomTypeListener (RoomSimPage* o) : owner (o) {}
        void parameterChanged (const juce::String&, float newValue) override
        {
            if (owner != nullptr)
            {
                int idx = juce::roundToInt (newValue);
                if (idx >= 0 && idx < numRoomTypes)
                {
                    juce::MessageManager::callAsync ([this, idx]
                    {
                        if (owner != nullptr)
                        {
                            owner->currentRoomType = idx;
                            owner->currentPresetIndex = 0;
                            owner->roomButtons[idx].setToggleState (true, juce::dontSendNotification);
                            owner->updatePresetName();
                        }
                    });
                }
            }
        }
    };
    RoomTypeListener roomTypeListener { this };

    // =====================================================================
    // Layout constants
    // =====================================================================
    static constexpr int   presetStripH  = 36;
    static constexpr float topProportion = 0.55f;
    static constexpr int   roomBtnStripH = 44;
    static constexpr int   knobColumnW   = 120;

    // =====================================================================
    // Colors (coherent with ReverbPage)
    // =====================================================================
    const juce::Colour gridBackground  = juce::Colour::fromRGB (28, 30, 38);
    const juce::Colour goldAccent      = juce::Colour::fromRGB (230, 176, 46);
    const juce::Colour knobTrack       = juce::Colour::fromRGB (55, 57, 62);
    const juce::Colour silverTop       = juce::Colour::fromRGB (132, 136, 142);
    const juce::Colour silverBottom    = juce::Colour::fromRGB (102, 106, 112);
    const juce::Colour presetStripBg   = juce::Colour::fromRGB (35, 37, 42);
    const juce::Colour presetBtnColour = juce::Colour::fromRGB (48, 50, 56);
    const juce::Colour presetNameBg    = juce::Colour::fromRGB (38, 40, 46);
    const juce::Colour roomBtnInactive = juce::Colour::fromRGB (40, 42, 48);
    const juce::Colour roomBtnActive   = juce::Colour::fromRGB (55, 57, 64);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoomSimPage)
};
