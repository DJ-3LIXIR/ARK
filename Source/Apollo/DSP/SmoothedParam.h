#pragma once

#include <JuceHeader.h>

/**
    Wraps a juce::SmoothedValue that reads its target from an
    std::atomic<float>* (APVTS raw parameter pointer) once per block.
*/
class SmoothedParam
{
public:
    SmoothedParam() = default;

    void prepare (double sampleRate, int /*samplesPerBlock*/)
    {
        sr = sampleRate;
        smoothed.reset (sampleRate, rampTimeSec);
    }

    void setSource (std::atomic<float>* paramPtr)
    {
        source = paramPtr;
        if (source != nullptr)
            smoothed.setCurrentAndTargetValue (source->load());
    }

    /** Set the smoothing ramp time in seconds. */
    void setSmoothingTime (float seconds)
    {
        rampTimeSec = seconds;
        smoothed.reset (sr, rampTimeSec);
    }

    /** Call once at the start of each processBlock to update the target. */
    void updateTarget()
    {
        if (source != nullptr)
            smoothed.setTargetValue (source->load());
    }

    /** Call once at the start of each processBlock with an explicit value. */
    void updateTarget (float value)
    {
        smoothed.setTargetValue (value);
    }

    /** Get the next smoothed sample. */
    float getNextValue() { return smoothed.getNextValue(); }

    /** Get the current value without advancing. */
    float getCurrentValue() const { return smoothed.getCurrentValue(); }

    /** Skip smoothing and set immediately. */
    void setCurrentAndTargetValue (float v) { smoothed.setCurrentAndTargetValue (v); }

    /** True if the value is still ramping. */
    bool isSmoothing() const { return smoothed.isSmoothing(); }

    void reset (float value)
    {
        smoothed.setCurrentAndTargetValue (value);
    }

private:
    std::atomic<float>* source = nullptr;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothed;
    double sr = 44100.0;
    float rampTimeSec = 0.02f; // 20ms default
};
