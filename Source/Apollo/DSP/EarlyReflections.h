#pragma once

#include <JuceHeader.h>

/**
    Multi-tap stereo early reflections generator.
    Supports up to 24 configurable taps with per-tap delay and stereo gain.
    Attack controls fade-in of earlier taps; distance scales tap spacing.
*/
class EarlyReflections
{
public:
    static constexpr int maxTaps = 24;

    struct Tap
    {
        float delayMs  = 0.0f;
        float gainL    = 0.0f;
        float gainR    = 0.0f;
    };

    EarlyReflections() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;

        // Max early reflection delay: taps up to 170ms * distance scale 4.0 = 680ms
        // Allocate 800ms to be safe
        int maxDelaySamples = juce::roundToInt (0.8f * (float) sr) + 1;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;

        delayL.setMaximumDelayInSamples (maxDelaySamples);
        delayR.setMaximumDelayInSamples (maxDelaySamples);
        delayL.prepare (spec);
        delayR.prepare (spec);
        delayL.reset();
        delayR.reset();
    }

    /** Set the tap pattern (room type dependent). */
    void setPattern (const Tap* newTaps, int numTaps)
    {
        jassert (numTaps <= maxTaps);
        activeTaps = juce::jmin (numTaps, maxTaps);
        for (int i = 0; i < activeTaps; ++i)
            taps[i] = newTaps[i];
    }

    /** Set attack (0-1). 0 = all taps at full, 1 = earliest taps heavily attenuated. */
    void setAttack (float attack01)
    {
        attack = juce::jlimit (0.0f, 1.0f, attack01);
    }

    /** Set distance (0-1). Scales tap delay times and attenuates with 1/r. */
    void setDistance (float distance01)
    {
        distance = juce::jlimit (0.0f, 1.0f, distance01);
    }

    void processSample (float inputL, float inputR, float& outL, float& outR)
    {
        delayL.pushSample (0, inputL);
        delayR.pushSample (0, inputR);

        outL = 0.0f;
        outR = 0.0f;

        if (activeTaps == 0) return;

        float delayScale = 1.0f + distance * 3.0f;
        float distanceAtten = 1.0f / (1.0f + distance * 2.0f);

        float maxDelay = (float) (sr * 0.78); // stay within allocated 800ms buffer

        for (int i = 0; i < activeTaps; ++i)
        {
            float tapDelay = taps[i].delayMs * delayScale;
            float delaySamples = tapDelay * 0.001f * (float) sr;
            delaySamples = juce::jlimit (1.0f, maxDelay, delaySamples);

            // Attack envelope: earlier taps (smaller index) get more attenuation
            float tapPosition = (float) i / (float) activeTaps;
            float attackEnv = 1.0f - attack * (1.0f - tapPosition);
            attackEnv = juce::jmax (attackEnv, 0.0f);

            float gl = taps[i].gainL * attackEnv * distanceAtten;
            float gr = taps[i].gainR * attackEnv * distanceAtten;

            outL += delayL.popSample (0, delaySamples, false) * gl;
            outR += delayR.popSample (0, delaySamples, false) * gr;
        }

        // Normalize output so early reflections sit at roughly the same level as the FDN
        float normGain = 1.0f / std::sqrt ((float) activeTaps);
        outL *= normGain;
        outR *= normGain;
    }

    void reset()
    {
        delayL.reset();
        delayR.reset();
    }

private:
    double sr = 44100.0;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL { 44100 * 2 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayR { 44100 * 2 };

    std::array<Tap, maxTaps> taps {};
    int activeTaps = 0;
    float attack = 0.0f;
    float distance = 0.5f;
};
