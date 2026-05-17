#pragma once

#include <JuceHeader.h>
#include "SmoothedParam.h"
#include "PreDelayLine.h"
#include "RoomSimPresets.h"

/**
    Room simulator engine using the image-source method for early reflections
    and an 8-line FDN for late reverb with allpass diffusion.

    Signal chain:
    Input * inputGain -> PreDelay -> split:
      - 6 image-source reflections (walls, floor, ceiling) -> earlyOut
      - allpass diffusion -> 8-line FDN -> lateOut
    -> mix early + late -> width -> mono -> phase -> dry/wet -> * outputGain
*/
class RoomSimEngine
{
public:
    RoomSimEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void updateParameters (
        bool on, float inputGain, float outputGain,
        float roomSize, float absorption, float diffusion,
        float predelay, float decay, float width,
        float earlyLate, float dry, float wet,
        int roomType, bool phaseInvert, bool mono, bool lock
    );

    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples);

private:
    // Image-source reflections (6 first-order: floor, ceiling, 4 walls)
    struct Reflection
    {
        float delaySamplesL = 0.0f;
        float delaySamplesR = 0.0f;
        float gainL = 0.0f;
        float gainR = 0.0f;
    };
    static constexpr int numReflections = 6;
    std::array<Reflection, numReflections> reflections;

    // Delay lines for image-source reflections (L + R per reflection)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> reflDelayL { 44100 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> reflDelayR { 44100 };

    // Late reverb: 8-line FDN
    static constexpr int lateN = 8;
    static constexpr float lateBaseDelays[lateN] = {
        887.0f, 1013.0f, 1153.0f, 1327.0f,
        1489.0f, 1657.0f, 1823.0f, 1999.0f
    };

    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>, lateN> lateDelays {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536)
    };

    std::array<float, lateN> lateFeedbackGains {};
    std::array<float, lateN> lateDampState {};
    std::array<float, lateN> lateDelayLengths {};
    float lateDampCoeff = 0.5f;

    // Late FDN input/output decorrelation
    std::array<float, lateN> lateInputL {}, lateInputR {};
    std::array<float, lateN> lateOutputL {}, lateOutputR {};

    // Allpass diffusion chain (4 stages per channel)
    struct AllpassSection
    {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 8192 };
        float state = 0.0f;
        float coeff = 0.5f;
        float delaySamples = 100.0f;
    };
    std::array<AllpassSection, 4> diffusionL, diffusionR;

    // Pre-delay
    PreDelayLine rsPredelayL, rsPredelayR;

    // Parameters
    bool bypassed = true;
    bool phaseInvert = false;
    bool monoOutput = false;
    bool lockParams = false;

    SmoothedParam dryGainSmoothed, wetGainSmoothed;
    SmoothedParam inputGainSmoothed, outputGainSmoothed;
    SmoothedParam earlyLateSmoothed;
    SmoothedParam widthSmoothed;

    double sr = 44100.0;
    int currentRoomType = -1;

    void calculateImageSources (float roomSizeMeters, float absorption01, int roomType);
    void setupLateFDN (float roomSizeMeters, float decay, float absorption01, int roomType);
    void setupDiffusion (float diffusion01);
    void setupLateInputOutputGains();
};
