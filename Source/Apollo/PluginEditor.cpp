/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const juce::Colour arkGold = juce::Colour::fromRGB (230, 176, 46);
const juce::Colour leadSilverTop = juce::Colour::fromRGB (132, 136, 142);
const juce::Colour leadSilverBottom = juce::Colour::fromRGB (102, 106, 112);
const juce::Colour darkText = juce::Colour::fromRGB (20, 24, 28);
const juce::Colour tabInactive = juce::Colour::fromRGB (60, 62, 66);
const juce::Colour tabActive = juce::Colour::fromRGB (42, 44, 48);
}

//==============================================================================
APOLLOAudioProcessorEditor::APOLLOAudioProcessorEditor (APOLLOAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), reverbPage (p, p.apvts, p.wetLevelAtomic), roomSimPage (p, p.apvts), settingsPage (p)
{
    auto& lf = juce::LookAndFeel::getDefaultLookAndFeel();
    lf.setColour (juce::ResizableWindow::backgroundColourId, arkGold);
    lf.setColour (juce::DocumentWindow::backgroundColourId, arkGold);
    lf.setColour (juce::TextButton::buttonColourId, arkGold);
    lf.setColour (juce::TextButton::buttonOnColourId, arkGold.brighter (0.10f));
    lf.setColour (juce::TextButton::textColourOffId, darkText);
    lf.setColour (juce::TextButton::textColourOnId, darkText);
    lf.setColour (juce::ComboBox::backgroundColourId, arkGold);
    lf.setColour (juce::ComboBox::textColourId, darkText);
    if (auto* v4 = dynamic_cast<juce::LookAndFeel_V4*> (&lf))
    {
        auto scheme = v4->getCurrentColourScheme();
        scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::UIColour::widgetBackground, arkGold);
        scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::UIColour::defaultText, darkText);
        v4->setColourScheme (scheme);
    }

    // Set up pages array
    pages.add (&reverbPage);
    pages.add (&roomSimPage);
    pages.add (&settingsPage);

    for (auto* page : pages)
        addChildComponent (page);

    // Create tab buttons
    for (int i = 0; i < tabNames.size(); ++i)
    {
        auto* btn = tabButtons.add (new juce::TextButton (tabNames[i]));
        btn->setClickingTogglesState (false);
        btn->setColour (juce::TextButton::buttonColourId, tabInactive);
        btn->setColour (juce::TextButton::buttonOnColourId, tabActive);
        btn->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.6f));
        btn->setColour (juce::TextButton::textColourOnId, arkGold);
        btn->onClick = [this, i] { switchToPage (i); };
        addAndMakeVisible (btn);
    }

    // Show saved page
    switchToPage (audioProcessor.savedPageIndex);

    setSize (1200, 800);
}

APOLLOAudioProcessorEditor::~APOLLOAudioProcessorEditor()
{
}

//==============================================================================
void APOLLOAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark lead-silver body
    auto area = getLocalBounds().toFloat();
    juce::ColourGradient leadGradient (leadSilverTop, 0.0f, 0.0f,
                                       leadSilverBottom, 0.0f, area.getBottom(), false);
    g.setGradientFill (leadGradient);
    g.fillAll();

    // Soft top sheen
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.14f), 0.0f, 0.0f,
                                juce::Colours::white.withAlpha (0.0f), 0.0f, area.getHeight() * 0.35f, false);
    g.setGradientFill (sheen);
    g.fillRect (area.withHeight (area.getHeight() * 0.35f));

    // Tab bar background
    g.setColour (juce::Colour::fromRGB (30, 32, 36));
    g.fillRect (0, 0, getWidth(), tabBarHeight);

    // Gold accent line under tabs
    g.setColour (arkGold);
    g.fillRect (0, tabBarHeight - 2, getWidth(), 2);
}

void APOLLOAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto tabArea = area.removeFromTop (tabBarHeight);

    // Lay out tab buttons evenly
    int tabWidth = tabArea.getWidth() / tabNames.size();
    for (int i = 0; i < tabButtons.size(); ++i)
        tabButtons[i]->setBounds (i * tabWidth, 0, tabWidth, tabBarHeight - 2);

    // Pages fill the rest
    for (auto* page : pages)
        page->setBounds (area);
}

void APOLLOAudioProcessorEditor::parentHierarchyChanged()
{
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        window->setUsingNativeTitleBar (false);
        window->setTitleBarHeight (30);
        window->setBackgroundColour (arkGold);
        window->setColour (juce::ResizableWindow::backgroundColourId, arkGold);
        window->setColour (juce::DocumentWindow::backgroundColourId, arkGold);
        window->setColour (juce::DocumentWindow::textColourId, darkText);

        if (window->getWidth() < 1200 || window->getHeight() < 800)
            window->setSize (1200, 800);
    }
}

void APOLLOAudioProcessorEditor::switchToPage (int pageIndex)
{
    if (pageIndex < 0 || pageIndex >= pages.size())
        return;

    currentPage = pageIndex;
    audioProcessor.savedPageIndex = pageIndex;

    // Show/hide pages
    for (int i = 0; i < pages.size(); ++i)
        pages[i]->setVisible (i == currentPage);

    // Update tab button states
    for (int i = 0; i < tabButtons.size(); ++i)
    {
        bool active = (i == currentPage);
        tabButtons[i]->setToggleState (active, juce::dontSendNotification);
        tabButtons[i]->setColour (juce::TextButton::buttonColourId, active ? tabActive : tabInactive);
        tabButtons[i]->setColour (juce::TextButton::textColourOffId,
                                  active ? arkGold : juce::Colours::white.withAlpha (0.6f));
    }
}
