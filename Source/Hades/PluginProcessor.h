/*
  ==============================================================================
    HADES - Plugin Processor Header
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class HadesAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    HadesAudioProcessor();
    ~HadesAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // APVTS
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // RMS metering for reactive display
    std::atomic<float> rmsLevel { 0.0f };

    // Power state per section — written by editor, read by processBlock
    std::atomic<bool> guitarActive { true };
    std::atomic<bool> bassActive   { true };
    std::atomic<bool> cabActive    { true };

    // Preset loading
    void loadGuitarPreset  (int presetIndex);
    void loadBassPreset    (int presetIndex);
    void loadCabinetPreset (int presetIndex);

    // Last selected preset index per page (0=guitar,1=bass,2=cabinet), persisted in state
    int savedPresetIndex[3] { 0, 0, 0 };

    // Last selected page (0=guitar,1=bass,2=cabinet,3=settings), persisted in state
    int savedPageIndex { 0 };

private:
    //==============================================================================
    // Guitar tone stack filters (L and R per channel)
    juce::dsp::IIR::Filter<float> guitarBassFilter,     guitarBassFilterR;
    juce::dsp::IIR::Filter<float> guitarMidsFilter,     guitarMidsFilterR;
    juce::dsp::IIR::Filter<float> guitarTrebleFilter,   guitarTrebleFilterR;
    juce::dsp::IIR::Filter<float> guitarPresenceFilter, guitarPresenceFilterR;

    // Bass tone stack filters
    juce::dsp::IIR::Filter<float> bassLowFilter,   bassLowFilterR;
    juce::dsp::IIR::Filter<float> bassLoMidFilter, bassLoMidFilterR;
    juce::dsp::IIR::Filter<float> bassHiMidFilter, bassHiMidFilterR;
    juce::dsp::IIR::Filter<float> bassHighFilter,  bassHighFilterR;

    // Cabinet low cut filter (stereo)
    juce::dsp::StateVariableTPTFilter<float> cabLowCutFilter;

    // Cabinet colour filters (4 biquads per cab type, L and R)
    juce::dsp::IIR::Filter<float> cabFilter1, cabFilter1R;
    juce::dsp::IIR::Filter<float> cabFilter2, cabFilter2R;
    juce::dsp::IIR::Filter<float> cabFilter3, cabFilter3R;
    juce::dsp::IIR::Filter<float> cabFilter4, cabFilter4R;

    // Air reverb delay buffers
    std::array<float, 4096> airDelayBufferL {};
    std::array<float, 4096> airDelayBufferR {};
    int airDelayWritePos { 0 };

    // Cached sample rate
    double currentSampleRate { 44100.0 };

    // Last known cab type to avoid redundant filter updates
    int lastCabType { -1 };

    //==============================================================================
    // Internal processing methods
    void processGuitar  (juce::AudioBuffer<float>& buffer);
    void processBass    (juce::AudioBuffer<float>& buffer);
    void processCabinet (juce::AudioBuffer<float>& buffer);
    void updateCabinetFilters (int cabType);

    // Helper to set APVTS parameter with host automation gesture
    void setParameterValue (const juce::String& paramId, float value);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HadesAudioProcessor)
};
