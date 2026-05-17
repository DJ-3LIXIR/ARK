#pragma once

#include <JuceHeader.h>

/**
    Stereo width processor using mid/side encoding.
    Includes a mono maker crossover filter that sums
    frequencies below a cutoff to mono.
*/
class StereoWidthProcessor
{
public:
    StereoWidthProcessor() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;

        monoLPF_L.prepare (spec);
        monoLPF_R.prepare (spec);
        monoHPF_L.prepare (spec);
        monoHPF_R.prepare (spec);

        monoLPF_L.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        monoLPF_R.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        monoHPF_L.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        monoHPF_R.setType (juce::dsp::StateVariableTPTFilterType::highpass);

        updateMonoMakerFreq();
    }

    /** Width in percent: 0% = mono, 100% = normal, 200% = extra wide. */
    void setWidth (float widthPercent)
    {
        sideGain = widthPercent / 100.0f;
    }

    /** Set mono maker crossover frequency in Hz. 0 = no effect. */
    void setMonoMakerFreq (float freqHz)
    {
        monoFreq = juce::jmax (freqHz, 0.0f);
        updateMonoMakerFreq();
    }

    void setMonoMakerEnabled (bool enabled) { monoEnabled = enabled; }

    void processSample (float& left, float& right)
    {
        // Width via mid/side
        float mid  = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        side *= sideGain;
        left  = mid + side;
        right = mid - side;

        // Mono maker: sum low frequencies to mono
        if (monoEnabled && monoFreq > 0.0f)
        {
            float lowL = monoLPF_L.processSample (0, left);
            float lowR = monoLPF_R.processSample (0, right);
            float highL = monoHPF_L.processSample (0, left);
            float highR = monoHPF_R.processSample (0, right);

            float monoLow = (lowL + lowR) * 0.5f;
            left  = monoLow + highL;
            right = monoLow + highR;
        }
    }

    void reset()
    {
        monoLPF_L.reset();
        monoLPF_R.reset();
        monoHPF_L.reset();
        monoHPF_R.reset();
    }

private:
    void updateMonoMakerFreq()
    {
        float freq = juce::jmax (monoFreq, 20.0f);
        freq = juce::jmin (freq, (float) sr * 0.45f);

        monoLPF_L.setCutoffFrequency (freq);
        monoLPF_R.setCutoffFrequency (freq);
        monoHPF_L.setCutoffFrequency (freq);
        monoHPF_R.setCutoffFrequency (freq);
    }

    double sr = 44100.0;
    float sideGain = 1.0f;
    float monoFreq = 0.0f;
    bool monoEnabled = false;

    juce::dsp::StateVariableTPTFilter<float> monoLPF_L, monoLPF_R;
    juce::dsp::StateVariableTPTFilter<float> monoHPF_L, monoHPF_R;
};
