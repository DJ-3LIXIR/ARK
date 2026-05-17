#pragma once

#include <JuceHeader.h>

/**
    Per-sample LFO for modulating FDN delay line lengths.
    Supports Sine, Square, and Noise waveforms.
    Each FDN line uses a phase-offset copy for decorrelation.
*/
class ModulationLFO
{
public:
    enum Waveform { Sine = 0, Square, Noise };

    ModulationLFO() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        phase = 0.0f;
        updatePhaseInc();
        noiseState = 0.0f;
        noiseSmoothCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 20.0f / (float) sr);
    }

    void setFrequency (float hz)
    {
        freq = hz;
        updatePhaseInc();
    }

    void setDepth (float depth01)    { depth = depth01; }
    void setWaveform (Waveform w)    { waveform = w; }

    /**
        Returns the modulation value for a given FDN line index.
        The output is in the range [-depth, +depth] where depth is
        the max excursion in samples (set externally via setDepth).
        Phase offset per line = lineIndex * 2pi/numLines for decorrelation.
    */
    float getNextSample (int lineIndex = 0, int numLines = 16)
    {
        float offsetPhase = phase + (float) lineIndex * juce::MathConstants<float>::twoPi / (float) numLines;

        // Wrap to [0, 1)
        offsetPhase -= std::floor (offsetPhase);

        float val = 0.0f;

        switch (waveform)
        {
            case Sine:
                val = std::sin (offsetPhase * juce::MathConstants<float>::twoPi);
                break;

            case Square:
                val = offsetPhase < 0.5f ? 1.0f : -1.0f;
                break;

            case Noise:
                // Smoothed random noise (unique per line via seed offset)
                val = getSmoothedNoise (lineIndex);
                break;
        }

        return val * depth;
    }

    /** Advance the LFO phase. Call once per sample AFTER reading all lines. */
    void advance()
    {
        phase += phaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Update noise states
        updateNoiseStates();
    }

    void reset()
    {
        phase = 0.0f;
        noiseState = 0.0f;
        for (auto& s : lineNoiseStates)
            s = 0.0f;
    }

private:
    void updatePhaseInc()
    {
        phaseInc = (float) (freq / sr);
    }

    void updateNoiseStates()
    {
        // Generate new random target periodically
        noiseCounter++;
        if (noiseCounter >= noisePeriod)
        {
            noiseCounter = 0;
            for (int i = 0; i < 16; ++i)
                noiseTargets[i] = noiseRng.nextFloat() * 2.0f - 1.0f;
        }

        // Smooth toward targets
        for (int i = 0; i < 16; ++i)
            lineNoiseStates[i] += noiseSmoothCoeff * (noiseTargets[i] - lineNoiseStates[i]);
    }

    float getSmoothedNoise (int lineIndex) const
    {
        if (lineIndex < 0 || lineIndex >= 16) return 0.0f;
        return lineNoiseStates[lineIndex];
    }

    double sr = 44100.0;
    float freq = 0.5f;
    float phase = 0.0f;
    float phaseInc = 0.0f;
    float depth = 0.0f;
    Waveform waveform = Sine;

    // Noise generation
    juce::Random noiseRng;
    float noiseState = 0.0f;
    float noiseSmoothCoeff = 0.01f;
    int noiseCounter = 0;
    int noisePeriod = 64;  // generate new random value every 64 samples
    std::array<float, 16> lineNoiseStates {};
    std::array<float, 16> noiseTargets {};
};
