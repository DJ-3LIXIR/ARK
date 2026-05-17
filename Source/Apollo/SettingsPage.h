#pragma once

#include <JuceHeader.h>

class APOLLOAudioProcessor;

class ApolloSettingsPage : public juce::Component,
                     public juce::Timer
{
public:
    ApolloSettingsPage (APOLLOAudioProcessor& p)
        : audioProcessor (p)
    {
        // =================================================================
        // Section headers
        // =================================================================
        addSection (pluginInfoHeader,  "PLUGIN INFO");
        addSection (audioInfoHeader,   "AUDIO INFO");
        addSection (performanceHeader, "PERFORMANCE");
        addSection (utilityHeader,     "UTILITY");
        addSection (aboutHeader,       "ABOUT");

        // =================================================================
        // Plugin Info labels
        // =================================================================
        addInfoRow (pluginNameLabel,  pluginNameValue,  "Plugin",    "APOLLO");
        addInfoRow (companyLabel,    companyValue,     "Company",   "3LIXIR MUSIC");
        addInfoRow (websiteLabel,    websiteValue,     "Website",   "www.3LIXIRMUSIC.com");
        addInfoRow (versionLabel,    versionValue,     "Version",   "1.0.0");
        addInfoRow (formatLabel,     formatValue,      "Format",    getWrapperType());

        // =================================================================
        // Audio Info labels (updated by timer)
        // =================================================================
        addInfoRow (sampleRateLabel, sampleRateValue, "Sample Rate", "---");
        addInfoRow (bufferSizeLabel, bufferSizeValue, "Buffer Size", "---");
        addInfoRow (latencyLabel,    latencyValue,    "Latency",     "---");
        addInfoRow (channelLabel,    channelValue,    "Channels",    "---");

        // =================================================================
        // Performance labels (updated by timer)
        // =================================================================
        addInfoRow (processingLabel, processingValue, "Processing", "---");

        // =================================================================
        // Utility buttons
        // =================================================================
        setupButton (resetAllButton,     "Reset All Parameters");
        setupButton (resetReverbButton,  "Reset Reverb");
        setupButton (resetRoomSimButton, "Reset Room Sim");

        resetAllButton.onClick     = [this] { resetAllParameters(); };
        resetReverbButton.onClick  = [this] { resetReverbParameters(); };
        resetRoomSimButton.onClick = [this] { resetRoomSimParameters(); };

        // =================================================================
        // About labels
        // =================================================================
        aboutText1.setText ("Built with JUCE Framework", juce::dontSendNotification);
        aboutText1.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));
        aboutText1.setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (aboutText1);

        aboutText2.setText (juce::String (juce::CharPointer_UTF8 ("\xc2\xa9")) + " 2025 3LIXIR MUSIC. All rights reserved.", juce::dontSendNotification);
        aboutText2.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.35f));
        aboutText2.setFont (juce::FontOptions (11.0f));
        addAndMakeVisible (aboutText2);

        startTimerHz (2);
    }

    ~ApolloSettingsPage() override
    {
        stopTimer();
    }

    void timerCallback() override
    {
        double sr = audioProcessor.getSampleRate();
        int bs = audioProcessor.getBlockSize();
        int lat = audioProcessor.getLatencySamples();

        sampleRateValue.setText (sr > 0 ? juce::String ((int) sr) + " Hz" : "N/A",
                                 juce::dontSendNotification);
        bufferSizeValue.setText (bs > 0 ? juce::String (bs) + " samples" : "N/A",
                                 juce::dontSendNotification);
        latencyValue.setText (juce::String (lat) + " samples", juce::dontSendNotification);

        auto layout = audioProcessor.getBusesLayout();
        int inCh  = layout.getMainInputChannels();
        int outCh = layout.getMainOutputChannels();
        channelValue.setText (juce::String (inCh) + " in / " + juce::String (outCh) + " out",
                              juce::dontSendNotification);

        processingValue.setText (outCh > 1 ? "Stereo" : "Mono", juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setColour (bgColour);
        g.fillRect (bounds);

        // Two-column layout
        auto inner = bounds.reduced (sideMargin, topMargin);
        int colW = (inner.getWidth() - colGap) / 2;

        // Left column dividers
        auto leftCol = inner.withWidth (colW);
        drawSectionDivider (g, leftCol.getX(), leftCol.getRight(), pluginInfoHeader.getBottom() + 4);
        drawSectionDivider (g, leftCol.getX(), leftCol.getRight(), audioInfoHeader.getBottom() + 4);

        // Right column dividers
        auto rightCol = inner.withX (inner.getX() + colW + colGap).withWidth (colW);
        drawSectionDivider (g, rightCol.getX(), rightCol.getRight(), performanceHeader.getBottom() + 4);
        drawSectionDivider (g, rightCol.getX(), rightCol.getRight(), utilityHeader.getBottom() + 4);
        drawSectionDivider (g, rightCol.getX(), rightCol.getRight(), aboutHeader.getBottom() + 4);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto inner = bounds.reduced (sideMargin, topMargin);

        int colW = (inner.getWidth() - colGap) / 2;
        int leftX = inner.getX();
        int rightX = inner.getX() + colW + colGap;

        int headerH = 22;
        int rowH = 24;
        int sectionGap = 28;

        // =================================================================
        // Left column
        // =================================================================
        int y = inner.getY();

        // Plugin Info
        pluginInfoHeader.setBounds (leftX, y, colW, headerH);
        y += headerH + 12;

        layoutInfoRow (pluginNameLabel, pluginNameValue, leftX, y, colW, rowH); y += rowH;
        layoutInfoRow (companyLabel,    companyValue,    leftX, y, colW, rowH); y += rowH;
        layoutInfoRow (websiteLabel,    websiteValue,    leftX, y, colW, rowH); y += rowH;
        layoutInfoRow (versionLabel,    versionValue,    leftX, y, colW, rowH); y += rowH;
        layoutInfoRow (formatLabel,     formatValue,     leftX, y, colW, rowH); y += rowH;

        y += sectionGap;

        // Audio Info
        audioInfoHeader.setBounds (leftX, y, colW, headerH);
        y += headerH + 12;

        layoutInfoRow (sampleRateLabel, sampleRateValue, leftX, y, colW, rowH); y += rowH;
        layoutInfoRow (bufferSizeLabel, bufferSizeValue, leftX, y, colW, rowH); y += rowH;
        layoutInfoRow (latencyLabel,    latencyValue,    leftX, y, colW, rowH); y += rowH;
        layoutInfoRow (channelLabel,    channelValue,    leftX, y, colW, rowH); y += rowH;

        // =================================================================
        // Right column
        // =================================================================
        y = inner.getY();

        // Performance
        performanceHeader.setBounds (rightX, y, colW, headerH);
        y += headerH + 12;

        layoutInfoRow (processingLabel, processingValue, rightX, y, colW, rowH); y += rowH;

        y += sectionGap;

        // Utility
        utilityHeader.setBounds (rightX, y, colW, headerH);
        y += headerH + 16;

        int btnW = 220;
        int btnH = 32;
        int btnGap = 10;

        resetAllButton.setBounds     (rightX, y, btnW, btnH); y += btnH + btnGap;
        resetReverbButton.setBounds  (rightX, y, btnW, btnH); y += btnH + btnGap;
        resetRoomSimButton.setBounds (rightX, y, btnW, btnH); y += btnH;

        y += sectionGap;

        // About
        aboutHeader.setBounds (rightX, y, colW, headerH);
        y += headerH + 12;

        aboutText1.setBounds (rightX, y, colW, 18); y += 20;
        aboutText2.setBounds (rightX, y, colW, 18);
    }

private:
    APOLLOAudioProcessor& audioProcessor;

    // =================================================================
    // Helpers
    // =================================================================
    void addSection (juce::Label& header, const juce::String& text)
    {
        header.setText (text, juce::dontSendNotification);
        header.setColour (juce::Label::textColourId, goldAccent);
        header.setFont (juce::FontOptions (14.0f));
        addAndMakeVisible (header);
    }

    void addInfoRow (juce::Label& label, juce::Label& value,
                     const juce::String& labelText, const juce::String& valueText)
    {
        label.setText (labelText, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));
        label.setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (label);

        value.setText (valueText, juce::dontSendNotification);
        value.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
        value.setFont (juce::FontOptions (13.0f));
        value.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (value);
    }

    void layoutInfoRow (juce::Label& label, juce::Label& value, int x, int y, int w, int h)
    {
        label.setBounds (x, y, w / 2, h);
        value.setBounds (x + w / 2, y, w / 2, h);
    }

    void setupButton (juce::TextButton& btn, const juce::String& text)
    {
        btn.setButtonText (text);
        btn.setColour (juce::TextButton::buttonColourId, buttonColour);
        btn.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.85f));
        addAndMakeVisible (btn);
    }

    void drawSectionDivider (juce::Graphics& g, int x1, int x2, int y)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine (y, (float) x1, (float) x2);
    }

    juce::String getWrapperType()
    {
        return juce::AudioProcessor::getWrapperTypeDescription (audioProcessor.wrapperType);
    }

    // =================================================================
    // Reset functions
    // =================================================================
    void resetReverbParameters()
    {
        auto& apvts = audioProcessor.apvts;
        const juce::String reverbParams[] = {
            "attack", "size", "density", "decay", "distance", "predelay",
            "dry", "wet", "freeze", "quality", "modSpeed", "modDepth",
            "smoothing", "earlyLate", "width", "monoMaker", "reverbOn",
            "roomType", "decayMode", "predelayMode", "monoMakerOn", "outputEqOn", "modSource"
        };

        for (auto& id : reverbParams)
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (p->getDefaultValue());

        for (int i = 1; i <= 8; ++i)
        {
            for (auto suffix : { "On", "Gain", "Q", "Freq" })
                if (auto* p = apvts.getParameter ("eqBand" + juce::String (i) + suffix))
                    p->setValueNotifyingHost (p->getDefaultValue());
        }

        for (int i = 1; i <= 5; ++i)
            if (auto* p = apvts.getParameter ("dampDotFreq" + juce::String (i)))
                p->setValueNotifyingHost (p->getDefaultValue());
    }

    void resetRoomSimParameters()
    {
        auto& apvts = audioProcessor.apvts;
        const juce::String rsParams[] = {
            "rsOn", "rsInputGain", "rsOutputGain", "rsRoomSize",
            "rsAbsorption", "rsDiffusion", "rsPredelay", "rsDecay",
            "rsWidth", "rsEarlyLate", "rsDry", "rsWet",
            "rsRoomType", "rsPhase", "rsMono", "rsLock"
        };

        for (auto& id : rsParams)
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (p->getDefaultValue());
    }

    void resetAllParameters()
    {
        resetReverbParameters();
        resetRoomSimParameters();
    }

    // =================================================================
    // Components
    // =================================================================

    // Section headers
    juce::Label pluginInfoHeader, audioInfoHeader, performanceHeader, utilityHeader, aboutHeader;

    // Plugin Info
    juce::Label pluginNameLabel, pluginNameValue;
    juce::Label companyLabel,    companyValue;
    juce::Label websiteLabel,    websiteValue;
    juce::Label versionLabel,    versionValue;
    juce::Label formatLabel,     formatValue;

    // Audio Info
    juce::Label sampleRateLabel, sampleRateValue;
    juce::Label bufferSizeLabel, bufferSizeValue;
    juce::Label latencyLabel,    latencyValue;
    juce::Label channelLabel,    channelValue;

    // Performance
    juce::Label processingLabel, processingValue;

    // Utility
    juce::TextButton resetAllButton;
    juce::TextButton resetReverbButton;
    juce::TextButton resetRoomSimButton;

    // About
    juce::Label aboutText1, aboutText2;

    // =================================================================
    // Layout constants
    // =================================================================
    static constexpr int sideMargin = 60;
    static constexpr int topMargin  = 40;
    static constexpr int colGap     = 80;

    // =================================================================
    // Colors
    // =================================================================
    const juce::Colour bgColour     = juce::Colour::fromRGB (28, 30, 38);
    const juce::Colour goldAccent   = juce::Colour::fromRGB (230, 176, 46);
    const juce::Colour buttonColour = juce::Colour::fromRGB (48, 50, 56);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApolloSettingsPage)
};
