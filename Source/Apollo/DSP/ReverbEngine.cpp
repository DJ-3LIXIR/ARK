#include "ReverbEngine.h"

ReverbEngine::ReverbEngine() = default;

void ReverbEngine::prepare (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    predelayLineL.prepare (sr, 200.0f);
    predelayLineR.prepare (sr, 200.0f);
    earlyReflections.prepare (sr);
    fdnReverb.prepare (sr);
    modulationLFO.prepare (sr);
    stereoWidth.prepare (sr);
    outputEQ.prepare (sr, samplesPerBlock);

    dryGain.prepare (sr, samplesPerBlock);
    wetGain.prepare (sr, samplesPerBlock);
    earlyLateBalance.prepare (sr, samplesPerBlock);

    dryGain.setCurrentAndTargetValue (1.0f);
    wetGain.setCurrentAndTargetValue (0.5f);
    earlyLateBalance.setCurrentAndTargetValue (0.5f);

    dryBuffer.setSize (2, samplesPerBlock);

    currentRoomType = 0;
    earlyReflections.setPattern (reverbRoomPresets[0].taps, reverbRoomPresets[0].numTaps);
}

void ReverbEngine::reset()
{
    predelayLineL.reset();
    predelayLineR.reset();
    earlyReflections.reset();
    fdnReverb.reset();
    modulationLFO.reset();
    stereoWidth.reset();
    outputEQ.reset();
    dryBuffer.clear();
}

void ReverbEngine::updateParameters (
    float attack, float size, float density, float decay,
    float distance, float predelay, float dry, float wet,
    bool freeze, bool reverbOn, int roomType,
    float quality, float modSpeed, float modDepth, int modSource,
    float smoothing, float earlyLate, float width,
    float monoMaker, bool monoMakerOn,
    bool outputEqOn, const bool eqBandOn[8],
    const float eqBandGain[8], const float eqBandQ[8])
{
    bypassed = !reverbOn;
    if (bypassed) return;

    // Get the room type preset
    int rtIdx = juce::jlimit (0, 13, roomType);
    const auto& preset = reverbRoomPresets[rtIdx];

    // If room type changed, update early reflection tap pattern
    if (rtIdx != currentRoomType)
    {
        currentRoomType = rtIdx;
        earlyReflections.setPattern (preset.taps, preset.numTaps);
    }

    // Smoothing time: 5ms at 0%, 100ms at 100%
    float smoothTime = 0.005f + (smoothing / 100.0f) * 0.095f;

    // Dry/Wet gains
    dryGain.setSmoothingTime (smoothTime);
    wetGain.setSmoothingTime (smoothTime);
    earlyLateBalance.setSmoothingTime (smoothTime);

    dryGain.updateTarget (dry / 100.0f);
    wetGain.updateTarget (wet / 100.0f);
    earlyLateBalance.updateTarget (earlyLate / 100.0f);

    // Pre-delay (with room type offset)
    float totalPredelay = predelay + preset.predelayOffset;
    totalPredelay = juce::jlimit (0.0f, 200.0f, totalPredelay);
    predelayLineL.setDelay (totalPredelay);
    predelayLineR.setDelay (totalPredelay);

    // Early reflections
    earlyReflections.setAttack (attack / 100.0f);
    // Apply preset early reflection density scaling
    earlyReflections.setDistance (distance / 100.0f * preset.erDensity);

    // FDN Reverb
    // Attack also controls FDN build-up (map to input gain ramping)
    float attackScale = 1.0f - (attack / 100.0f) * 0.8f;
    fdnReverb.setSize ((size / 100.0f) * preset.sizeScale * (0.5f + attackScale * 0.5f));
    fdnReverb.setDensity ((density / 100.0f) * preset.densityScale);
    fdnReverb.setDecayTime (decay * preset.decayScale);
    // Distance also pushes high-frequency damping on the FDN
    float distanceDamp = preset.dampingHigh + (distance / 100.0f) * 0.25f;
    fdnReverb.setDamping (juce::jmin (distanceDamp, 0.95f), preset.dampingLow);
    fdnReverb.setFreeze (freeze);
    fdnReverb.setQuality (quality / 100.0f);

    // Modulation
    modulationLFO.setFrequency (modSpeed);
    float maxModExcursion = 12.0f * ((float) sr / 44100.0f); // 12 samples at 44.1kHz
    modulationLFO.setDepth ((modDepth / 100.0f) * maxModExcursion * preset.modDepthScale);
    modulationLFO.setWaveform (static_cast<ModulationLFO::Waveform> (juce::jlimit (0, 2, modSource)));

    // Apply preset stereo spread to stereo width processor
    stereoWidth.setWidth (width * preset.stereoSpread);
    stereoWidth.setMonoMakerFreq (monoMaker);
    stereoWidth.setMonoMakerEnabled (monoMakerOn);

    // Output EQ
    outputEQ.setEnabled (outputEqOn);
    for (int i = 0; i < 8; ++i)
    {
        outputEQ.setBandEnabled (i, eqBandOn[i]);
        outputEQ.setBandGain (i, eqBandGain[i]);
        outputEQ.setBandQ (i, eqBandQ[i]);
    }
}

void ReverbEngine::processBlock (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (bypassed) return;

    int numChannels = buffer.getNumChannels();
    bool isStereo = numChannels >= 2;

    // Ensure dry buffer is big enough
    if (dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (2, numSamples, false, false, true);

    // Copy dry input
    for (int ch = 0; ch < juce::jmin (numChannels, 2); ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    auto* dataL = buffer.getWritePointer (0);
    auto* dataR = isStereo ? buffer.getWritePointer (1) : dataL;

    auto* dryL = dryBuffer.getReadPointer (0);
    auto* dryR = isStereo ? dryBuffer.getReadPointer (1) : dryL;

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = dataL[s];
        float inR = isStereo ? dataR[s] : inL;

        // Pre-delay
        float pdL = predelayLineL.processSample (inL);
        float pdR = predelayLineR.processSample (inR);

        // Early reflections
        float erL = 0.0f, erR = 0.0f;
        earlyReflections.processSample (pdL, pdR, erL, erR);

        // Update FDN modulation from LFO
        for (int line = 0; line < 16; ++line)
            fdnReverb.setModulation (line, modulationLFO.getNextSample (line, 16));
        modulationLFO.advance();

        // Late reverb (FDN)
        float lateL = 0.0f, lateR = 0.0f;
        fdnReverb.processSample (pdL, pdR, lateL, lateR);

        // Mix early + late based on balance
        float bal = earlyLateBalance.getNextValue();
        float wetL = erL * (1.0f - bal) + lateL * bal;
        float wetR = erR * (1.0f - bal) + lateR * bal;

        // Stereo width + mono maker
        stereoWidth.processSample (wetL, wetR);

        // Dry/wet mix
        float dG = dryGain.getNextValue();
        float wG = wetGain.getNextValue();

        dataL[s] = dryL[s] * dG + wetL * wG;
        if (isStereo)
            dataR[s] = dryR[s] * dG + wetR * wG;
    }

    // Output EQ (block-based for efficiency)
    outputEQ.processBlock (buffer, numSamples);

    // Soft-clip output to prevent digital crackling from runaway signals
    for (int ch = 0; ch < juce::jmin (numChannels, 2); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int s = 0; s < numSamples; ++s)
            data[s] = std::tanh (data[s]);
    }
}
