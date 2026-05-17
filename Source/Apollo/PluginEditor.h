/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ReverbPage.h"
#include "RoomSimPage.h"
#include "SettingsPage.h"

//==============================================================================
class APOLLOAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    APOLLOAudioProcessorEditor (APOLLOAudioProcessor&);
    ~APOLLOAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    void switchToPage (int pageIndex);

    APOLLOAudioProcessor& audioProcessor;

    // Tab buttons
    juce::OwnedArray<juce::TextButton> tabButtons;
    juce::StringArray tabNames { "REVERB", "ROOM SIM", "SETTINGS" };

    // Pages (initialized in constructor with APVTS references)
    ReverbPage   reverbPage;
    RoomSimPage  roomSimPage;
    ApolloSettingsPage settingsPage;
    juce::Array<juce::Component*> pages;

    int currentPage = 0;

    static constexpr int tabBarHeight = 40;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APOLLOAudioProcessorEditor)
};
