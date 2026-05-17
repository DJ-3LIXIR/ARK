#pragma once

#include <JuceHeader.h>

/**
    16-channel Feedback Delay Network reverb core.

    Uses prime-number delay lengths, Householder mixing matrix,
    one-pole damping filters, and allpass diffusers per line.
    Supports freeze mode, quality scaling (4/8/12/16 active lines),
    and external modulation of delay line lengths.
*/
class FDNReverb
{
public:
    static constexpr int N = 16;

    FDNReverb() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;

        // Scale base delay lengths for current sample rate
        const float srRatio = (float) sr / 44100.0f;
        for (int i = 0; i < N; ++i)
            scaledBaseDelays[i] = baseDelays44k[i] * srRatio;

        // Max delay: base * maxSizeScale (accounting for preset multipliers) + modulation headroom
        // Presets can multiply user size by up to 2.0x, and sizeScale maps 0-1 to 0.3-3.0,
        // so effective max sizeScale = 0.3 + 2.0 * 2.7 = 5.7, round up to 6.0 for safety
        int maxDelaySamples = juce::roundToInt (scaledBaseDelays[N - 1] * 6.5f) + 128;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;

        for (int i = 0; i < N; ++i)
        {
            delays[i].setMaximumDelayInSamples (maxDelaySamples);
            delays[i].prepare (spec);
            delays[i].reset();
        }

        reset();
        setupInputOutputGains();
    }

    void setDecayTime (float seconds)
    {
        decayTimeSec = juce::jmax (seconds, 0.1f);
        updateFeedbackGains();
    }

    void setSize (float size01)
    {
        // Scale: 0% -> 0.3x, 100% -> 3.0x
        // Clamp input — presets can multiply user values > 1.0
        size01 = juce::jlimit (0.0f, 2.0f, size01);
        if (std::abs (size01 - lastSize01) < 0.001f) return;
        lastSize01 = size01;
        sizeScale = 0.3f + size01 * 2.7f;
        updateDelayLengths();
        updateFeedbackGains();
    }

    void setDensity (float density01)
    {
        if (std::abs (density01 - lastDensity01) < 0.001f) return;
        lastDensity01 = density01;
        float coeff = density01 * 0.85f;
        for (int i = 0; i < N; ++i)
            for (auto& ap : diffusers[i])
            {
                ap.coeff = coeff;
                ap.buffer = ap.buffer * coeff;
            }
    }

    void setDamping (float highDamp, float lowDamp)
    {
        // High-frequency damping (one-pole lowpass): higher = darker
        dampCoeffHigh = juce::jlimit (0.0f, 0.98f, highDamp * 1.4f);
        // Low-frequency damping (one-pole highpass): higher = thinner
        dampCoeffLow = juce::jlimit (0.0f, 0.98f, lowDamp * 2.0f);
    }

    void setFreeze (bool freeze)
    {
        frozen = freeze;
        if (frozen)
        {
            for (int i = 0; i < N; ++i)
                feedbackGains[i] = 1.0f;
        }
        else
        {
            updateFeedbackGains();
        }
    }

    void setQuality (float quality01)
    {
        // Map 0-100% to 4/8/12/16 active lines
        if (quality01 < 0.25f)       activeLines = 4;
        else if (quality01 < 0.50f)  activeLines = 8;
        else if (quality01 < 0.75f)  activeLines = 12;
        else                         activeLines = 16;
    }

    /** Set the modulation offset in samples for a specific delay line. */
    void setModulation (int lineIndex, float offsetSamples)
    {
        if (lineIndex >= 0 && lineIndex < N)
            modOffsets[lineIndex] = offsetSamples;
    }

    void processSample (float inputL, float inputR, float& outL, float& outR)
    {
        // Read from delay lines
        float maxAllowedDelay = scaledBaseDelays[N - 1] * 6.0f;
        std::array<float, N> readValues;
        for (int i = 0; i < activeLines; ++i)
        {
            float totalDelay = currentDelayLengths[i] + modOffsets[i];
            totalDelay = juce::jlimit (1.0f, maxAllowedDelay, totalDelay);
            readValues[i] = delays[i].popSample (0, totalDelay);
        }
        for (int i = activeLines; i < N; ++i)
            readValues[i] = 0.0f;

        // Apply damping filters
        if (!frozen)
        {
            for (int i = 0; i < activeLines; ++i)
            {
                // High-frequency damping (one-pole lowpass)
                dampStateHigh[i] = readValues[i] + dampCoeffHigh * (dampStateHigh[i] - readValues[i]);
                readValues[i] = dampStateHigh[i];

                // Low-frequency damping (one-pole highpass = input - lowpass)
                dampStateLow[i] += dampCoeffLow * (readValues[i] - dampStateLow[i]);
                readValues[i] = readValues[i] - dampStateLow[i];
            }
        }

        // Apply feedback gains
        for (int i = 0; i < activeLines; ++i)
            readValues[i] *= feedbackGains[i];

        // Allpass diffusers (2 cascaded per line)
        for (int i = 0; i < activeLines; ++i)
        {
            for (auto& ap : diffusers[i])
            {
                float v = readValues[i] - ap.coeff * ap.buffer;
                readValues[i] = ap.buffer + ap.coeff * v;
                ap.buffer = v;
            }
        }

        // Householder mixing matrix: H = I - (2/N) * 1*1^T
        // Efficiently: mean = sum/N, then each = each - 2*mean
        {
            float sum = 0.0f;
            for (int i = 0; i < activeLines; ++i)
                sum += readValues[i];
            float mean2 = 2.0f * sum / (float) activeLines;
            for (int i = 0; i < activeLines; ++i)
                readValues[i] -= mean2;
        }

        // Inject input (distributed across lines)
        for (int i = 0; i < activeLines; ++i)
        {
            float in = inputL * inputGainsL[i] + inputR * inputGainsR[i];
            readValues[i] += in;
        }

        // Write back to delay lines
        for (int i = 0; i < activeLines; ++i)
            delays[i].pushSample (0, readValues[i]);
        for (int i = activeLines; i < N; ++i)
            delays[i].pushSample (0, 0.0f);

        // Sum output
        outL = 0.0f;
        outR = 0.0f;
        for (int i = 0; i < activeLines; ++i)
        {
            outL += readValues[i] * outputGainsL[i];
            outR += readValues[i] * outputGainsR[i];
        }

        // Normalize output based on active line count
        float normGain = std::sqrt (16.0f / (float) activeLines);
        outL *= normGain * 0.25f;
        outR *= normGain * 0.25f;
    }

    void reset()
    {
        for (int i = 0; i < N; ++i)
        {
            delays[i].reset();
            dampStateHigh[i] = 0.0f;
            dampStateLow[i] = 0.0f;
            modOffsets[i] = 0.0f;
            for (auto& ap : diffusers[i])
                ap.buffer = 0.0f;
        }
        feedbackState.fill (0.0f);
        updateDelayLengths();
        updateFeedbackGains();
    }

private:
    void updateDelayLengths()
    {
        float maxAllowed = scaledBaseDelays[N - 1] * 6.0f; // stay within allocated buffer
        for (int i = 0; i < N; ++i)
            currentDelayLengths[i] = juce::jmin (scaledBaseDelays[i] * sizeScale, maxAllowed);
    }

    void updateFeedbackGains()
    {
        if (frozen) return;

        for (int i = 0; i < N; ++i)
        {
            float delayInSeconds = currentDelayLengths[i] / (float) sr;
            // RT60: gain = 10^(-3 * delay / decayTime)
            feedbackGains[i] = std::pow (10.0f, -3.0f * delayInSeconds / decayTimeSec);
            feedbackGains[i] = juce::jmin (feedbackGains[i], 0.9995f);
        }
    }

    void setupInputOutputGains()
    {
        // Distribute input and output across lines with decorrelation
        // Odd lines get more L, even lines get more R
        for (int i = 0; i < N; ++i)
        {
            float angle = (float) i / (float) N * juce::MathConstants<float>::pi;
            inputGainsL[i] = std::cos (angle) * 0.5f + 0.5f;
            inputGainsR[i] = std::sin (angle) * 0.5f + 0.5f;

            // Output gains: different rotation for decorrelation
            float outAngle = ((float) i + 0.5f) / (float) N * juce::MathConstants<float>::pi;
            outputGainsL[i] = std::cos (outAngle);
            outputGainsR[i] = std::sin (outAngle);
        }
    }

    // Prime-number base delays at 44.1kHz
    static constexpr float baseDelays44k[N] = {
        601.0f,  691.0f,  773.0f,  839.0f,
        929.0f,  1049.0f, 1151.0f, 1277.0f,
        1373.0f, 1493.0f, 1609.0f, 1741.0f,
        1867.0f, 1993.0f, 2129.0f, 2269.0f
    };

    double sr = 44100.0;

    // Delay lines with Lagrange3rd for smooth modulation
    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>, N> delays {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536),
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> (65536)
    };

    std::array<float, N> scaledBaseDelays {};
    std::array<float, N> currentDelayLengths {};
    std::array<float, N> feedbackGains {};
    std::array<float, N> feedbackState {};
    std::array<float, N> modOffsets {};

    // Damping
    std::array<float, N> dampStateHigh {};
    std::array<float, N> dampStateLow {};
    float dampCoeffHigh = 0.5f;
    float dampCoeffLow = 0.1f;

    // Allpass diffusers: 2 cascaded per line
    struct AllpassDiffuser
    {
        float buffer = 0.0f;
        float coeff = 0.0f;
    };
    std::array<std::array<AllpassDiffuser, 2>, N> diffusers {};

    float sizeScale = 1.0f;
    float lastSize01 = -1.0f;
    float lastDensity01 = -1.0f;
    float decayTimeSec = 1.1f;
    int activeLines = 16;
    bool frozen = false;

    // Input/output decorrelation
    std::array<float, N> inputGainsL {}, inputGainsR {};
    std::array<float, N> outputGainsL {}, outputGainsR {};
};
