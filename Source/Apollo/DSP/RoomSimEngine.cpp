#include "RoomSimEngine.h"

RoomSimEngine::RoomSimEngine() = default;

void RoomSimEngine::prepare (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 1;

    // Reflection delay lines
    int maxReflDelay = juce::roundToInt (0.5f * (float) sr); // 500ms max
    reflDelayL.setMaximumDelayInSamples (maxReflDelay);
    reflDelayR.setMaximumDelayInSamples (maxReflDelay);
    reflDelayL.prepare (spec);
    reflDelayR.prepare (spec);

    // Late FDN
    float srRatio = (float) sr / 44100.0f;
    for (int i = 0; i < lateN; ++i)
    {
        int maxDelay = juce::roundToInt (lateBaseDelays[i] * srRatio * 5.0f) + 64;
        lateDelays[i].setMaximumDelayInSamples (maxDelay);
        lateDelays[i].prepare (spec);
    }

    // Diffusion allpass chains
    for (int i = 0; i < 4; ++i)
    {
        diffusionL[i].delay.setMaximumDelayInSamples (8192);
        diffusionR[i].delay.setMaximumDelayInSamples (8192);
        diffusionL[i].delay.prepare (spec);
        diffusionR[i].delay.prepare (spec);
    }

    // Pre-delay
    rsPredelayL.prepare (sr, 200.0f);
    rsPredelayR.prepare (sr, 200.0f);

    // Smoothed params
    dryGainSmoothed.prepare (sr, samplesPerBlock);
    wetGainSmoothed.prepare (sr, samplesPerBlock);
    inputGainSmoothed.prepare (sr, samplesPerBlock);
    outputGainSmoothed.prepare (sr, samplesPerBlock);
    earlyLateSmoothed.prepare (sr, samplesPerBlock);
    widthSmoothed.prepare (sr, samplesPerBlock);

    dryGainSmoothed.setCurrentAndTargetValue (1.0f);
    wetGainSmoothed.setCurrentAndTargetValue (0.5f);
    inputGainSmoothed.setCurrentAndTargetValue (1.0f);
    outputGainSmoothed.setCurrentAndTargetValue (1.0f);
    earlyLateSmoothed.setCurrentAndTargetValue (0.5f);
    widthSmoothed.setCurrentAndTargetValue (1.0f);

    setupLateInputOutputGains();
    currentRoomType = -1;
}

void RoomSimEngine::reset()
{
    reflDelayL.reset();
    reflDelayR.reset();

    for (auto& d : lateDelays)
        d.reset();
    lateDampState.fill (0.0f);

    for (int i = 0; i < 4; ++i)
    {
        diffusionL[i].delay.reset();
        diffusionR[i].delay.reset();
        diffusionL[i].state = 0.0f;
        diffusionR[i].state = 0.0f;
    }

    rsPredelayL.reset();
    rsPredelayR.reset();
}

void RoomSimEngine::updateParameters (
    bool on, float inputGain, float outputGain,
    float roomSize, float absorption, float diffusion,
    float predelay, float decay, float width,
    float earlyLate, float dry, float wet,
    int roomType, bool phase, bool mono, bool lock)
{
    bypassed = !on;
    if (bypassed) return;

    phaseInvert = phase;
    monoOutput = mono;

    // If lock is on, ignore parameter changes
    if (lock && lockParams) return;
    lockParams = lock;

    int rtIdx = juce::jlimit (0, 7, roomType);

    inputGainSmoothed.updateTarget (inputGain / 100.0f);
    outputGainSmoothed.updateTarget (outputGain / 100.0f);
    dryGainSmoothed.updateTarget (dry / 100.0f);
    wetGainSmoothed.updateTarget (wet / 100.0f);
    earlyLateSmoothed.updateTarget (earlyLate / 100.0f);
    widthSmoothed.updateTarget (width / 100.0f);

    rsPredelayL.setDelay (predelay);
    rsPredelayR.setDelay (predelay);

    // Calculate image-source reflections
    calculateImageSources (roomSize, absorption / 100.0f, rtIdx);

    // Setup late FDN
    setupLateFDN (roomSize, decay, absorption / 100.0f, rtIdx);

    // Setup diffusion
    setupDiffusion (diffusion / 100.0f);

    currentRoomType = rtIdx;
}

void RoomSimEngine::calculateImageSources (float roomSizeMeters, float absorption01, int roomType)
{
    const auto& preset = roomSimPresets[roomType];

    // Calculate actual room dimensions
    float w = roomSizeMeters * preset.widthRatio;   // width (meters)
    float h = roomSizeMeters * preset.heightRatio;   // height (meters)
    float d = roomSizeMeters * preset.depthRatio;    // depth (meters)

    constexpr float speedOfSound = 343.0f; // m/s

    // Listener at center, source offset slightly
    // 6 first-order reflections: left wall, right wall, floor, ceiling, front wall, back wall
    struct WallInfo {
        float distance;     // round-trip distance in meters
        float absCoeff;     // absorption coefficient
        float panL, panR;   // stereo panning
    };

    float userAbs = absorption01;
    float wallAbs  = preset.wallAbsorption * (0.5f + userAbs * 0.5f);
    float floorAbs = preset.floorAbsorption * (0.5f + userAbs * 0.5f);
    float ceilAbs  = preset.ceilingAbsorption * (0.5f + userAbs * 0.5f);

    WallInfo walls[6] = {
        { w,       wallAbs,  0.8f, 0.3f },  // left wall
        { w,       wallAbs,  0.3f, 0.8f },  // right wall
        { h,       floorAbs, 0.6f, 0.6f },  // floor
        { h,       ceilAbs,  0.6f, 0.6f },  // ceiling
        { d * 0.7f, wallAbs, 0.5f, 0.5f },  // front wall (closer)
        { d * 1.3f, wallAbs, 0.5f, 0.5f },  // back wall (farther)
    };

    for (int i = 0; i < numReflections; ++i)
    {
        float roundTripMeters = walls[i].distance * 2.0f;
        float delayMs = (roundTripMeters / speedOfSound) * 1000.0f;
        float delaySamples = delayMs * 0.001f * (float) sr;

        // Clamp to max delay line length
        delaySamples = juce::jmin (delaySamples, (float) sr * 0.45f);

        // Reflection gain: inverse distance * (1 - absorption)
        float distanceLoss = 1.0f / (1.0f + roundTripMeters * 0.1f);
        float absLoss = 1.0f - walls[i].absCoeff;
        float gain = distanceLoss * absLoss;

        // Slight L/R offset for stereo imaging
        float offsetMs = (i % 2 == 0) ? 0.3f : -0.3f;
        float offsetSamples = offsetMs * 0.001f * (float) sr;

        reflections[i].delaySamplesL = juce::jmax (delaySamples + offsetSamples, 1.0f);
        reflections[i].delaySamplesR = juce::jmax (delaySamples - offsetSamples, 1.0f);
        reflections[i].gainL = gain * walls[i].panL;
        reflections[i].gainR = gain * walls[i].panR;
    }
}

void RoomSimEngine::setupLateFDN (float roomSizeMeters, float decay, float absorption01, int roomType)
{
    const auto& preset = roomSimPresets[roomType];
    float srRatio = (float) sr / 44100.0f;
    float sizeScale = roomSizeMeters / 40.0f; // normalize to default 40m

    float effectiveDecay = decay * preset.lateDecayScale;
    effectiveDecay = juce::jmax (effectiveDecay, 0.1f);

    // Damping from absorption
    lateDampCoeff = juce::jlimit (0.0f, 0.95f, absorption01 * 0.8f);

    for (int i = 0; i < lateN; ++i)
    {
        float delayLen = lateBaseDelays[i] * srRatio * sizeScale;
        delayLen = juce::jmax (delayLen, 1.0f);
        lateDelayLengths[i] = delayLen;

        // Feedback gain for RT60
        float delayInSec = delayLen / (float) sr;
        lateFeedbackGains[i] = std::pow (10.0f, -3.0f * delayInSec / effectiveDecay);
        lateFeedbackGains[i] = juce::jmin (lateFeedbackGains[i], 0.999f);
    }
}

void RoomSimEngine::setupDiffusion (float diffusion01)
{
    // Allpass delay times (prime-ish values in samples, scaled)
    static const float apDelays[4] = { 142.0f, 107.0f, 379.0f, 277.0f };
    float srRatio = (float) sr / 44100.0f;
    float coeff = diffusion01 * 0.65f;

    for (int i = 0; i < 4; ++i)
    {
        float delaySamples = apDelays[i] * srRatio;
        diffusionL[i].delaySamples = delaySamples;
        diffusionR[i].delaySamples = delaySamples * 1.13f; // slight offset for stereo
        diffusionL[i].coeff = coeff;
        diffusionR[i].coeff = coeff;
    }
}

void RoomSimEngine::setupLateInputOutputGains()
{
    for (int i = 0; i < lateN; ++i)
    {
        float angle = (float) i / (float) lateN * juce::MathConstants<float>::pi;
        lateInputL[i] = std::cos (angle) * 0.5f + 0.5f;
        lateInputR[i] = std::sin (angle) * 0.5f + 0.5f;
        float outAngle = ((float) i + 0.5f) / (float) lateN * juce::MathConstants<float>::pi;
        lateOutputL[i] = std::cos (outAngle);
        lateOutputR[i] = std::sin (outAngle);
    }
}

void RoomSimEngine::processBlock (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (bypassed) return;

    int numChannels = buffer.getNumChannels();
    bool isStereo = numChannels >= 2;

    auto* dataL = buffer.getWritePointer (0);
    auto* dataR = isStereo ? buffer.getWritePointer (1) : dataL;

    for (int s = 0; s < numSamples; ++s)
    {
        float inGain = inputGainSmoothed.getNextValue();
        float outGain = outputGainSmoothed.getNextValue();
        float dryG = dryGainSmoothed.getNextValue();
        float wetG = wetGainSmoothed.getNextValue();
        float elBal = earlyLateSmoothed.getNextValue();
        float widthVal = widthSmoothed.getNextValue();

        float rawL = dataL[s];
        float rawR = isStereo ? dataR[s] : rawL;

        float inL = rawL * inGain;
        float inR = rawR * inGain;

        // Pre-delay
        float pdL = rsPredelayL.processSample (inL);
        float pdR = rsPredelayR.processSample (inR);

        // === Early reflections (image-source method) ===
        reflDelayL.pushSample (0, pdL);
        reflDelayR.pushSample (0, pdR);

        float earlyL = 0.0f, earlyR = 0.0f;
        for (int r = 0; r < numReflections; ++r)
        {
            earlyL += reflDelayL.popSample (0, reflections[r].delaySamplesL, false) * reflections[r].gainL;
            earlyR += reflDelayR.popSample (0, reflections[r].delaySamplesR, false) * reflections[r].gainR;
        }

        // === Diffusion allpass chain ===
        float diffL = pdL, diffR = pdR;
        for (int i = 0; i < 4; ++i)
        {
            // Left channel
            {
                auto& ap = diffusionL[i];
                ap.delay.pushSample (0, diffL);
                float delayed = ap.delay.popSample (0, ap.delaySamples);
                float v = diffL - ap.coeff * delayed;
                diffL = delayed + ap.coeff * v;
            }
            // Right channel
            {
                auto& ap = diffusionR[i];
                ap.delay.pushSample (0, diffR);
                float delayed = ap.delay.popSample (0, ap.delaySamples);
                float v = diffR - ap.coeff * delayed;
                diffR = delayed + ap.coeff * v;
            }
        }

        // === Late FDN reverb ===
        std::array<float, lateN> readVals;
        for (int i = 0; i < lateN; ++i)
            readVals[i] = lateDelays[i].popSample (0, lateDelayLengths[i]);

        // Damping
        for (int i = 0; i < lateN; ++i)
        {
            lateDampState[i] = readVals[i] + lateDampCoeff * (lateDampState[i] - readVals[i]);
            readVals[i] = lateDampState[i];
        }

        // Feedback
        for (int i = 0; i < lateN; ++i)
            readVals[i] *= lateFeedbackGains[i];

        // Householder mixing
        {
            float sum = 0.0f;
            for (int i = 0; i < lateN; ++i) sum += readVals[i];
            float mean2 = 2.0f * sum / (float) lateN;
            for (int i = 0; i < lateN; ++i) readVals[i] -= mean2;
        }

        // Inject diffused input
        for (int i = 0; i < lateN; ++i)
            readVals[i] += diffL * lateInputL[i] + diffR * lateInputR[i];

        // Write back
        for (int i = 0; i < lateN; ++i)
            lateDelays[i].pushSample (0, readVals[i]);

        // Sum late output
        float lateL = 0.0f, lateR = 0.0f;
        for (int i = 0; i < lateN; ++i)
        {
            lateL += readVals[i] * lateOutputL[i];
            lateR += readVals[i] * lateOutputR[i];
        }
        lateL *= 0.35f;
        lateR *= 0.35f;

        // === Mix early + late ===
        float wetL = earlyL * (1.0f - elBal) + lateL * elBal;
        float wetR = earlyR * (1.0f - elBal) + lateR * elBal;

        // === Stereo width (mid/side) ===
        {
            float mid  = (wetL + wetR) * 0.5f;
            float side = (wetL - wetR) * 0.5f;
            side *= widthVal;
            wetL = mid + side;
            wetR = mid - side;
        }

        // === Mono ===
        if (monoOutput)
        {
            float m = (wetL + wetR) * 0.5f;
            wetL = m;
            wetR = m;
        }

        // === Phase invert ===
        if (phaseInvert)
        {
            wetL = -wetL;
            wetR = -wetR;
        }

        // === Dry/wet mix ===
        float finalL = rawL * dryG + wetL * wetG;
        float finalR = rawR * dryG + wetR * wetG;

        // === Output gain + soft-clip to prevent digital crackling ===
        dataL[s] = std::tanh (finalL * outGain);
        if (isStereo)
            dataR[s] = std::tanh (finalR * outGain);
    }
}
