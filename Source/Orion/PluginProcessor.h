/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class OrionSoundEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    static constexpr int maxBands = 32;

    //==============================================================================
    OrionSoundEQAudioProcessor();
    ~OrionSoundEQAudioProcessor() override;

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

    // Audio level for GUI (thread-safe)
    float getCurrentRMSLevel() const { return currentRMSLevel.load(); }

    //==============================================================================
    // DSP control methods (called from Editor on timer)
    void setBandParameters (int bandIndex, float freq, float gainDB, float Q, int filterType);
    void setNumActiveBands (int count);
    void setOutputGain (float gainDB);
    void setBandMode (int modeId);           // 1 = single-band, 2 = multi-band
    void setSelectedBand (int bandIndex);    // which band is active in single-band mode
    void setBypassed (bool bypassed);        // true = bypass all EQ processing

    //==============================================================================
    // Saved UI state (persisted across close/open)
    // These are written by the editor and read back when a new editor is created.
    struct SavedBand
    {
        float frequency = 1000.0f;
        float gainDB    = 0.0f;
        float Q         = 1.0f;
        int   filterType = 0;
    };

    SavedBand savedBands[maxBands];
    int savedBandCount       = 8;
    int savedBandModeId      = 1;   // 1=single, 2=multi
    int savedPresetComboId   = 1;   // default (flat)
    float savedOutputGainDB  = 0.0f;
    bool stateHasBeenLoaded      = false;  // true once setStateInformation was called
    bool editorStateInitialized  = false;  // true once the editor timer has pushed state

    // User presets
    struct UserPreset
    {
        juce::String name;
        int bandCount = 0;
        int bandModeId = 1;
        SavedBand bands[maxBands];
    };
    std::vector<UserPreset> userPresets;
    bool userPresetsChanged = false;  // flag for editor to rebuild menu

private:
    std::atomic<float> currentRMSLevel { 0.0f };

    //==============================================================================
    // DSP members

    // Per-band parameter storage (written by editor, read by processor)
    struct BandParams
    {
        std::atomic<float> frequency { 1000.0f };
        std::atomic<float> gainDB    { 0.0f };
        std::atomic<float> Q         { 1.0f };
        std::atomic<int>   filterType { 0 };
    };

    BandParams bandParams[maxBands];
    std::atomic<int>   numActiveBands { 0 };
    std::atomic<float> outputGainDB   { 0.0f };
    std::atomic<int>   bandModeId     { 1 };     // 1 = single-band, 2 = multi-band
    std::atomic<int>   selectedBand   { -1 };    // selected band index for single-band mode
    std::atomic<bool>  eqBypassed     { false };  // true = bypass all EQ processing

    // IIR filters: [band][channel]  (stereo = 2 channels)
    juce::IIRFilter filters[maxBands][2];

    // Sample rate stored for coefficient updates
    double currentSampleRate = 44100.0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrionSoundEQAudioProcessor)
};
