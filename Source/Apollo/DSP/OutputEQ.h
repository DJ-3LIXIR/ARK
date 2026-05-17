#pragma once

#include <JuceHeader.h>

/**
    8-band parametric output EQ.
    Band types: HighPass, LowShelf, 4x Bell, HighShelf, LowPass
    Fixed center frequencies; configurable gain, Q, and on/off per band.
*/
class OutputEQ
{
public:
    static constexpr int numBands = 8;

    // Band center frequencies (mutable for interactive adjustment)
    float bandFreqs[numBands] = {
        40.0f, 100.0f, 250.0f, 800.0f, 2500.0f, 6000.0f, 10000.0f, 16000.0f
    };

    enum BandType
    {
        HighPass = 0, LowShelf, Bell, Bell2, Bell3, Bell4, HighShelf, LowPass
    };

    OutputEQ() = default;

    void prepare (double sampleRate, int /*samplesPerBlock*/)
    {
        sr = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;

        for (auto& band : bands)
        {
            band.filterL.prepare (spec);
            band.filterR.prepare (spec);
        }

        // Initialize with default coefficients
        for (int i = 0; i < numBands; ++i)
            updateCoefficients (i);
    }

    void setEnabled (bool enabled)           { masterEnabled = enabled; }
    void setBandEnabled (int idx, bool on)   { if (idx >= 0 && idx < numBands) bands[idx].enabled = on; }

    void setBandFrequency (int idx, float freqHz)
    {
        if (idx >= 0 && idx < numBands)
        {
            bandFreqs[idx] = freqHz;
            updateCoefficients (idx);
        }
    }

    float getBandFrequency (int idx) const
    {
        if (idx >= 0 && idx < numBands)
            return bandFreqs[idx];
        return 0.0f;
    }

    void setBandGain (int idx, float gainDb)
    {
        if (idx >= 0 && idx < numBands)
        {
            bands[idx].gainDb = gainDb;
            updateCoefficients (idx);
        }
    }

    void setBandQ (int idx, float q)
    {
        if (idx >= 0 && idx < numBands)
        {
            bands[idx].q = q;
            updateCoefficients (idx);
        }
    }

    void processSample (float& left, float& right)
    {
        if (!masterEnabled) return;

        for (int i = 0; i < numBands; ++i)
        {
            if (!bands[i].enabled) continue;
            left  = bands[i].filterL.processSample (left);
            right = bands[i].filterR.processSample (right);
        }
    }

    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (!masterEnabled) return;

        auto* dataL = buffer.getWritePointer (0);
        auto* dataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            float l = dataL[s];
            float r = dataR != nullptr ? dataR[s] : l;

            for (int i = 0; i < numBands; ++i)
            {
                if (!bands[i].enabled) continue;
                l = bands[i].filterL.processSample (l);
                r = bands[i].filterR.processSample (r);
            }

            dataL[s] = l;
            if (dataR != nullptr) dataR[s] = r;
        }
    }

    void reset()
    {
        for (auto& band : bands)
        {
            band.filterL.reset();
            band.filterR.reset();
        }
    }

private:
    void updateCoefficients (int idx)
    {
        float freq = bandFreqs[idx];
        float q    = bands[idx].q;
        float gain = juce::Decibels::decibelsToGain (bands[idx].gainDb);

        // Clamp frequency to Nyquist
        freq = juce::jmin (freq, (float) sr * 0.49f);

        juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

        switch (idx)
        {
            case 0: // HighPass 40Hz
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, freq, q);
                break;
            case 1: // LowShelf 100Hz
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, freq, q, gain);
                break;
            case 2: // Bell 250Hz
            case 3: // Bell 800Hz
            case 4: // Bell 2500Hz
            case 5: // Bell 6000Hz
                coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, freq, q, gain);
                break;
            case 6: // HighShelf 10kHz
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, freq, q, gain);
                break;
            case 7: // LowPass 16kHz
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, freq, q);
                break;
        }

        if (coeffs != nullptr)
        {
            *bands[idx].filterL.coefficients = *coeffs;
            *bands[idx].filterR.coefficients = *coeffs;
        }
    }

    struct Band
    {
        juce::dsp::IIR::Filter<float> filterL, filterR;
        bool enabled = true;
        float gainDb = 0.0f;
        float q = 0.707f;
    };

    std::array<Band, numBands> bands {};
    bool masterEnabled = true;
    double sr = 44100.0;
};
