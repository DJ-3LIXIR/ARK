/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSP/ReverbEngine.h"
#include "DSP/RoomSimEngine.h"

//==============================================================================
/**
*/
class APOLLOAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    APOLLOAudioProcessor();
    ~APOLLOAudioProcessor() override;

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

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Thread-safe level meter — written by audio thread, read by UI timer
    std::atomic<float> wetLevelAtomic { 0.0f };

    // Preset loading
    void loadReverbPreset (int category, int subPreset);
    void loadRoomSimPreset (int roomType, int subPreset);

    // Last selected preset index per page (0=reverb, 1=roomSim), persisted in state
    int savedPresetIndex[2] { 0, 0 };

    // Last selected page (0=reverb, 1=roomSim, 2=settings), persisted in state
    int savedPageIndex { 0 };

private:
    //==============================================================================
    // DSP Engines
    ReverbEngine reverbEngine;
    RoomSimEngine roomSimEngine;

    // Cached parameter pointers (reverb)
    std::atomic<float>* attackParam      = nullptr;
    std::atomic<float>* sizeParam        = nullptr;
    std::atomic<float>* densityParam     = nullptr;
    std::atomic<float>* decayParam       = nullptr;
    std::atomic<float>* distanceParam    = nullptr;
    std::atomic<float>* predelayParam    = nullptr;
    std::atomic<float>* dryParam         = nullptr;
    std::atomic<float>* wetParam         = nullptr;
    std::atomic<float>* freezeParam      = nullptr;
    std::atomic<float>* reverbOnParam    = nullptr;
    std::atomic<float>* roomTypeParam    = nullptr;
    std::atomic<float>* qualityParam     = nullptr;
    std::atomic<float>* modSpeedParam    = nullptr;
    std::atomic<float>* modDepthParam    = nullptr;
    std::atomic<float>* modSourceParam   = nullptr;
    std::atomic<float>* smoothingParam   = nullptr;
    std::atomic<float>* earlyLateParam   = nullptr;
    std::atomic<float>* widthParam       = nullptr;
    std::atomic<float>* monoMakerParam   = nullptr;
    std::atomic<float>* monoMakerOnParam = nullptr;
    std::atomic<float>* outputEqOnParam  = nullptr;

    std::atomic<float>* eqBandOnParams[8]   = {};
    std::atomic<float>* eqBandGainParams[8] = {};
    std::atomic<float>* eqBandQParams[8]    = {};
    std::atomic<float>* eqBandFreqParams[8] = {};

    // Cached parameter pointers (room sim)
    std::atomic<float>* rsOnParam          = nullptr;
    std::atomic<float>* rsInputGainParam   = nullptr;
    std::atomic<float>* rsOutputGainParam  = nullptr;
    std::atomic<float>* rsRoomSizeParam    = nullptr;
    std::atomic<float>* rsAbsorptionParam  = nullptr;
    std::atomic<float>* rsDiffusionParam   = nullptr;
    std::atomic<float>* rsPredelayParam    = nullptr;
    std::atomic<float>* rsDecayParam       = nullptr;
    std::atomic<float>* rsWidthParam       = nullptr;
    std::atomic<float>* rsEarlyLateParam   = nullptr;
    std::atomic<float>* rsDryParam         = nullptr;
    std::atomic<float>* rsWetParam         = nullptr;
    std::atomic<float>* rsRoomTypeParam    = nullptr;
    std::atomic<float>* rsPhaseParam       = nullptr;
    std::atomic<float>* rsMonoParam        = nullptr;
    std::atomic<float>* rsLockParam        = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (APOLLOAudioProcessor)
};
