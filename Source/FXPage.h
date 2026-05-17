#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Hades/PluginProcessor.h"
#include "Hades/PluginEditor.h"
#include "Apollo/PluginProcessor.h"
#include "Apollo/PluginEditor.h"
#include "Orion/PluginProcessor.h"
#include "Orion/PluginEditor.h"

//==============================================================================
// Invisible square button LookAndFeel — draws nothing, purely a click target
//==============================================================================
class InvisibleButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool, bool) override {}
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
};

//==============================================================================
// Single rack slot — holds all buttons, state, and plugin instance for one rack
//==============================================================================
struct RackSlot
{
    juce::TextButton selectButton;   // Invisible hit area over rack number for selection
    juce::TextButton pluginSelector;
    juce::TextButton powerButton;
    juce::TextButton closeButton;   // X button to delete rack
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    int selectedPlugin = 0;  // 0 = No Audio Effects, 1 = Hades, 2 = Apollo, 3 = Orion

    // Plugin processor instance (owned per-rack)
    std::unique_ptr<juce::AudioProcessor> pluginProcessor;
};

//==============================================================================
// Scrollable rack list — lives inside a Viewport
//==============================================================================
class RackListComponent : public juce::Component
{
public:
    RackListComponent (InvisibleButtonLookAndFeel& lnf);
    ~RackListComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void addRack();
    void removeRack (int index);
    int getNumRacks() const { return (int) racks.size(); }
    int getContentHeight() const;

    // Selection
    int selectedRackIndex = 0;
    void selectRack (int index);
    RackSlot* getSelectedRack();
    const RackSlot* getSelectedRack() const;
    RackSlot* getRack (int index);
    const RackSlot* getRack (int index) const;

    // Callbacks to FXPage
    std::function<void()> onSelectionChanged;
    std::function<void(int)> onPluginChanged;  // rackIndex
    std::function<void()> onRackStateChanged;   // PWR/Mute toggled or rack added/removed

    // Select All button
    juce::TextButton selectAllBtn;

private:
    InvisibleButtonLookAndFeel& invisibleButtonLnf;
    juce::OwnedArray<RackSlot> racks;
    juce::TextButton addRackBtn;

    // Layout constants
    static constexpr float RACK_NUMBER_W = 35.0f;
    static constexpr float SLOT_H = 50.0f;
    static constexpr float SLOT_SPACING = 56.0f;  // slotH + 6
    static constexpr float HEADER_H = 52.0f;      // RACK label + divider

    float getSlotStartX() const;
    float getSlotMaxW() const;
    float getFirstSlotY() const;

    void setupRackSlot (RackSlot* slot, int index);
    void onPluginSelectorClicked (int rackIndex);
    void onCloseClicked (int rackIndex);

    void drawRackSlot (juce::Graphics& g, int index, float slotY);

    juce::String getPluginName (int pluginId) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackListComponent)
};

//==============================================================================
// FX PAGE COMPONENT
//==============================================================================
class FXPage : public juce::Component
{
public:
    FXPage (ARKAudioProcessor* processor);
    ~FXPage() override;

    void paint (juce::Graphics& g) override;
    void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

    void setProcessor (ARKAudioProcessor* p) { processor = p; }

    // State serialization for preset save/load
    std::unique_ptr<juce::XmlElement> saveFXState() const;
    void restoreFXState (const juce::XmlElement* xml);

private:
    ARKAudioProcessor* processor = nullptr;

    InvisibleButtonLookAndFeel invisibleButtonLnf;

    // Scrollable rack list
    RackListComponent rackList;
    juce::Viewport rackViewport;

    // Display panel — shows the selected plugin's editor
    std::unique_ptr<juce::AudioProcessorEditor> currentEditor;
    juce::Rectangle<int> displayBounds;  // cached display area bounds

    void updateDisplayPanel();
    void createPluginForRack (int rackIndex, int pluginId);
    void syncFxChainToProcessor();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FXPage)
};
