#pragma once

#include <JuceHeader.h>

/**
    Simple pre-delay line wrapping juce::dsp::DelayLine.
    Supports up to 200ms delay with linear interpolation.
*/
class PreDelayLine
{
public:
    PreDelayLine() = default;

    void prepare (double sampleRate, float maxDelayMs = 200.0f)
    {
        sr = sampleRate;
        int maxSamples = juce::roundToInt (maxDelayMs * 0.001f * (float) sr) + 1;
        delay.setMaximumDelayInSamples (maxSamples);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;
        delay.prepare (spec);
        delay.reset();
    }

    void setDelay (float delayMs)
    {
        float delaySamples = delayMs * 0.001f * (float) sr;
        delay.setDelay (delaySamples);
    }

    float processSample (float input)
    {
        delay.pushSample (0, input);
        return delay.popSample (0);
    }

    void reset()
    {
        delay.reset();
    }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 44100 };
    double sr = 44100.0;
};
