#pragma once

#include <JuceHeader.h>
#include "SmoothedParam.h"
#include "PreDelayLine.h"
#include "EarlyReflections.h"
#include "FDNReverb.h"
#include "ModulationLFO.h"
#include "StereoWidthProcessor.h"
#include "OutputEQ.h"
#include "RoomTypePresets.h"

/**
    Top-level reverb engine that orchestrates the full reverb signal chain:
    Input -> PreDelay -> Early/Late split -> Width -> MonoMaker -> EQ -> Dry/Wet mix
*/
class ReverbEngine
{
public:
    ReverbEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    /** Call once per processBlock to update all parameters from APVTS values. */
    void updateParameters (
        float attack, float size, float density, float decay,
        float distance, float predelay, float dry, float wet,
        bool freeze, bool reverbOn, int roomType,
        float quality, float modSpeed, float modDepth, int modSource,
        float smoothing, float earlyLate, float width,
        float monoMaker, bool monoMakerOn,
        bool outputEqOn, const bool eqBandOn[8],
        const float eqBandGain[8], const float eqBandQ[8]
    );

    /** Process a stereo buffer in-place. */
    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples);

private:
    // Sub-components
    PreDelayLine predelayLineL, predelayLineR;
    EarlyReflections earlyReflections;
    FDNReverb fdnReverb;
    ModulationLFO modulationLFO;
    StereoWidthProcessor stereoWidth;
    OutputEQ outputEQ;

    // Smoothed parameters
    SmoothedParam dryGain, wetGain;
    SmoothedParam earlyLateBalance;

    // State
    bool bypassed = false;
    int currentRoomType = -1;
    double sr = 44100.0;

    // Dry buffer for mixing
    juce::AudioBuffer<float> dryBuffer;
};
