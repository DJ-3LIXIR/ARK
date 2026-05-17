#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

/**
    FrequencyResponseCalculator

    Computes biquad filter frequency response curves for visualization.
    Uses the standard biquad transfer function to evaluate magnitude at each frequency.
*/
class FrequencyResponseCalculator
{
public:
    /**
        Compute the magnitude response (in dB) of a biquad filter at a given frequency.

        Uses the transfer function: H(e^jω) = (b0 + b1*e^(-jω) + b2*e^(-2jω)) / (a0 + a1*e^(-jω) + a2*e^(-2jω))
        where ω = 2π*f/sampleRate

        @param coeffs  JUCE IIR filter coefficients
        @param frequency  Frequency in Hz
        @param sampleRate  Sample rate in Hz
        @return Magnitude response in dB
    */
    static float computeMagnitude(const juce::dsp::IIR::Coefficients<float>& coeffs,
                                 float frequency, float sampleRate)
    {
        // Clamp frequency to below Nyquist
        frequency = juce::jmin(frequency, sampleRate * 0.49f);
        frequency = juce::jmax(frequency, 0.1f);  // Avoid log(0)

        // Angular frequency ω = 2π*f/sr
        const float omega = juce::MathConstants<float>::twoPi * frequency / sampleRate;
        const float cosOmega = std::cos(omega);
        const float sinOmega = std::sin(omega);
        const float cos2Omega = std::cos(2.0f * omega);
        const float sin2Omega = std::sin(2.0f * omega);

        // Get coefficients
        const auto& b = coeffs.coefficients;
        const float b0 = b[0];
                const float b1 = b[1];
                const float b2 = b[2];
                const float a0 = 1.0f;   // JUCE normalizes so a0 = 1
                const float a1 = b[3];
                const float a2 = b[4];

        // Numerator: (b0 + b1*e^(-jω) + b2*e^(-2jω))
        // Real: b0 + b1*cos(ω) + b2*cos(2ω)
        // Imag: -b1*sin(ω) - b2*sin(2ω)
        const float numReal = b0 + b1 * cosOmega + b2 * cos2Omega;
        const float numImag = -b1 * sinOmega - b2 * sin2Omega;
        const float numMag = std::sqrt(numReal * numReal + numImag * numImag);

        // Denominator: (a0 + a1*e^(-jω) + a2*e^(-2jω))
        // Real: a0 + a1*cos(ω) + a2*cos(2ω)
        // Imag: -a1*sin(ω) - a2*sin(2ω)
        const float denReal = a0 + a1 * cosOmega + a2 * cos2Omega;
        const float denImag = -a1 * sinOmega - a2 * sin2Omega;
        const float denMag = std::sqrt(denReal * denReal + denImag * denImag);

        // Magnitude response
        const float mag = numMag / denMag;
        const float magDb = 20.0f * std::log10(mag + 1e-7f);  // Add epsilon to avoid log(0)

        return magDb;
    }

    /**
        Fill a frequency response array using given coefficients across the full frequency range.

        @param coeffs   IIR filter coefficients for the band
        @param magdB    Output array to fill with magnitude values (in dB)
        @param numPoints Number of frequency points to compute
        @param sampleRate Sample rate in Hz
    */
    static void fillFrequencyArray(const juce::dsp::IIR::Coefficients<float>& coeffs,
                                   std::vector<float>& magdB,
                                   int numPoints,
                                   float sampleRate)
    {
        magdB.resize(numPoints);

        // Frequency range: 20Hz to 20kHz (logarithmic spacing)
        const float logMin = std::log10(20.0f);
        const float logMax = std::log10(20000.0f);

        for (int i = 0; i < numPoints; ++i)
        {
            const float t = i / (float)(numPoints - 1);
            const float logFreq = logMin + t * (logMax - logMin);
            const float freq = std::pow(10.0f, logFreq);

            magdB[i] = computeMagnitude(coeffs, freq, sampleRate);
        }
    }

    /**
        Sum multiple frequency response curves into a composite curve (in dB).

        For series filters, magnitude responses multiply: |H_total| = |H1| * |H2| * ...
        In dB space: 20*log10(|H_total|) = 20*log10(|H1|) + 20*log10(|H2|) + ...
        So we sum the dB values.

        @param bandCurves  Vector of magnitude arrays (one per band, in dB)
        @param enabledBands Vector of bools indicating which bands are enabled
        @param sumCurve  Output array for the summed response (in dB)
    */
    static void sumFrequencyArrays(const std::vector<std::vector<float>>& bandCurves,
                                   const std::vector<bool>& enabledBands,
                                   std::vector<float>& sumCurve)
    {
        if (bandCurves.empty())
            return;

        const int numPoints = bandCurves[0].size();
        sumCurve.assign(numPoints, 0.0f);

        // Sum enabled band curves in dB space (for series filters, add dB values)
        for (size_t b = 0; b < bandCurves.size() && b < enabledBands.size(); ++b)
        {
            if (!enabledBands[b])
                continue;

            for (int i = 0; i < numPoints; ++i)
                sumCurve[i] += bandCurves[b][i];
        }
    }
};
