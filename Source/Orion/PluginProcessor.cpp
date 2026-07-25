/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OrionSoundEQAudioProcessor::OrionSoundEQAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

OrionSoundEQAudioProcessor::~OrionSoundEQAudioProcessor()
{
}

//==============================================================================
const juce::String OrionSoundEQAudioProcessor::getName() const
{
    return "Orion";
}

bool OrionSoundEQAudioProcessor::acceptsMidi() const { return false; }
bool OrionSoundEQAudioProcessor::producesMidi() const { return false; }
bool OrionSoundEQAudioProcessor::isMidiEffect() const { return false; }

double OrionSoundEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OrionSoundEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OrionSoundEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OrionSoundEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OrionSoundEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void OrionSoundEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void OrionSoundEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Reset all filters
    for (int b = 0; b < maxBands; ++b)
        for (int ch = 0; ch < 2; ++ch)
            filters[b][ch].reset();

    // Reset spectrum analyzer to the noise floor
    fftFifoIndex = 0;
    const juce::SpinLock::ScopedLockType sl (spectrumLock);
    for (int k = 0; k < numBins; ++k)
        spectrumDB[k] = -120.0f;
}

void OrionSoundEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool OrionSoundEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void OrionSoundEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int numChannels = juce::jmin ((int) totalNumInputChannels, 2);
    const int numSamples  = buffer.getNumSamples();

    // If bypassed, pass audio through with no processing and no RMS measurement
    if (eqBypassed.load())
    {
        currentRMSLevel.store(0.0f);
        return;
    }

    const int activeBands = numActiveBands.load();
    const double sr       = currentSampleRate;

    const int mode     = bandModeId.load();
    const int selBand  = selectedBand.load();

    // Update filter coefficients and process each active band
    for (int b = 0; b < activeBands && b < maxBands; ++b)
    {
        // In single-band mode, skip all bands except the selected one
        if (mode == 1 && selBand >= 0 && b != selBand)
            continue;
        float freq = bandParams[b].frequency.load();
        float gain = bandParams[b].gainDB.load();
        float q    = bandParams[b].Q.load();
        int   type = bandParams[b].filterType.load();

        // Guard against bad values
        freq = juce::jlimit (20.0f, 20000.0f, freq);
        q    = juce::jmax (0.1f, q);

        float gainLinear = juce::Decibels::decibelsToGain (gain);

        // Create coefficients based on filter type
        juce::IIRCoefficients coeffs;

        switch (type)
        {
            case 0: // Bell (Peak)
                coeffs = juce::IIRCoefficients::makePeakFilter (sr, freq, q, gainLinear);
                break;
            case 1: // Low Cut (High Pass)
                coeffs = juce::IIRCoefficients::makeHighPass (sr, freq, q);
                break;
            case 2: // High Cut (Low Pass)
                coeffs = juce::IIRCoefficients::makeLowPass (sr, freq, q);
                break;
            case 3: // Low Shelf
                coeffs = juce::IIRCoefficients::makeLowShelf (sr, freq, q, gainLinear);
                break;
            case 4: // High Shelf
                coeffs = juce::IIRCoefficients::makeHighShelf (sr, freq, q, gainLinear);
                break;
            case 5: // Notch
                coeffs = juce::IIRCoefficients::makeNotchFilter (sr, freq, q);
                break;
            default:
                coeffs = juce::IIRCoefficients::makePeakFilter (sr, freq, q, gainLinear);
                break;
        }

        // Apply coefficients and process each channel
        for (int ch = 0; ch < numChannels; ++ch)
        {
            filters[b][ch].setCoefficients (coeffs);
            filters[b][ch].processSamples (buffer.getWritePointer (ch), numSamples);
        }
    }

    // Apply output gain
    float outGain = juce::Decibels::decibelsToGain (outputGainDB.load());
    if (std::abs (outGain - 1.0f) > 0.0001f)
        buffer.applyGain (outGain);

    // Measure RMS level AFTER processing for GUI
    float maxRMS = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float rms = buffer.getRMSLevel (channel, 0, numSamples);
        if (rms > maxRMS)
            maxRMS = rms;
    }
    currentRMSLevel.store (maxRMS);

    // Feed post-EQ output into the spectrum analyzer (only when it's showing)
    if (spectrumActive.load() && numChannels > 0)
    {
        const float* chL = buffer.getReadPointer (0);
        const float* chR = (numChannels > 1) ? buffer.getReadPointer (1) : nullptr;
        for (int n = 0; n < numSamples; ++n)
        {
            float mono = (chR != nullptr) ? 0.5f * (chL[n] + chR[n]) : chL[n];
            pushSampleToFFT (mono);
        }
    }
}

//==============================================================================
// Spectrum analyzer
//==============================================================================

void OrionSoundEQAudioProcessor::pushSampleToFFT (float sample) noexcept
{
    fftFifo[fftFifoIndex++] = sample;
    if (fftFifoIndex >= fftSize)
    {
        fftFifoIndex = 0;
        computeSpectrum();
    }
}

void OrionSoundEQAudioProcessor::computeSpectrum() noexcept
{
    // Hann window into the real buffer; imaginary starts at zero.
    for (int i = 0; i < fftSize; ++i)
    {
        float w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * i / (fftSize - 1)));
        fftReal[i] = fftFifo[i] * w;
        fftImag[i] = 0.0f;
    }

    // In-place iterative radix-2 Cooley-Tukey FFT.
    // Bit-reversal permutation.
    for (int i = 1, j = 0; i < fftSize; ++i)
    {
        int bit = fftSize >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
        {
            std::swap (fftReal[i], fftReal[j]);
            std::swap (fftImag[i], fftImag[j]);
        }
    }

    for (int len = 2; len <= fftSize; len <<= 1)
    {
        float ang = -juce::MathConstants<float>::twoPi / (float) len;
        float wRealStep = std::cos (ang);
        float wImagStep = std::sin (ang);
        for (int i = 0; i < fftSize; i += len)
        {
            float wReal = 1.0f, wImag = 0.0f;
            for (int k = 0; k < len / 2; ++k)
            {
                int a = i + k;
                int b = i + k + len / 2;
                float tReal = fftReal[b] * wReal - fftImag[b] * wImag;
                float tImag = fftReal[b] * wImag + fftImag[b] * wReal;
                fftReal[b] = fftReal[a] - tReal;
                fftImag[b] = fftImag[a] - tImag;
                fftReal[a] += tReal;
                fftImag[a] += tImag;
                float nwReal = wReal * wRealStep - wImag * wImagStep;
                wImag = wReal * wImagStep + wImag * wRealStep;
                wReal = nwReal;
            }
        }
    }

    // Convert to dBFS magnitudes and smooth against the previously published frame.
    const juce::SpinLock::ScopedTryLockType sl (spectrumLock);
    if (! sl.isLocked())
        return;  // GUI is reading; skip this frame rather than block the audio thread

    const float norm = 2.0f / (float) fftSize;   // single-sided magnitude scaling
    for (int k = 0; k < numBins; ++k)
    {
        float mag = std::sqrt (fftReal[k] * fftReal[k] + fftImag[k] * fftImag[k]) * norm;
        float db  = juce::Decibels::gainToDecibels (mag, -120.0f);
        // Fast attack, slower release for a stable-looking analyzer.
        float prev = spectrumDB[k];
        spectrumDB[k] = (db > prev) ? db : prev + 0.25f * (db - prev);
    }
}

void OrionSoundEQAudioProcessor::copySpectrumDB (float* dest, int numValues) noexcept
{
    const juce::SpinLock::ScopedLockType sl (spectrumLock);
    int n = juce::jmin (numValues, numBins);
    for (int i = 0; i < n; ++i)
        dest[i] = spectrumDB[i];
    for (int i = n; i < numValues; ++i)
        dest[i] = -120.0f;
}

//==============================================================================
// DSP control methods
//==============================================================================

void OrionSoundEQAudioProcessor::setBandParameters (int bandIndex, float freq, float gain, float Q, int filterType)
{
    if (bandIndex < 0 || bandIndex >= maxBands)
        return;

    bandParams[bandIndex].frequency.store (freq);
    bandParams[bandIndex].gainDB.store (gain);
    bandParams[bandIndex].Q.store (Q);
    bandParams[bandIndex].filterType.store (filterType);
}

void OrionSoundEQAudioProcessor::setNumActiveBands (int count)
{
    numActiveBands.store (juce::jlimit (0, maxBands, count));
}

void OrionSoundEQAudioProcessor::setOutputGain (float gainDB)
{
    outputGainDB.store (gainDB);
}

void OrionSoundEQAudioProcessor::setBandMode (int modeId)
{
    bandModeId.store (modeId);
}

void OrionSoundEQAudioProcessor::setSelectedBand (int bandIndex)
{
    selectedBand.store (bandIndex);
}

void OrionSoundEQAudioProcessor::setBypassed (bool bypassed)
{
    eqBypassed.store (bypassed);
}

//==============================================================================
bool OrionSoundEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OrionSoundEQAudioProcessor::createEditor()
{
    return new OrionSoundEQAudioProcessorEditor (*this);
}

//==============================================================================
void OrionSoundEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement>("OrionSoundEQState");

    xml->setAttribute("bandCount",      savedBandCount);
    xml->setAttribute("bandModeId",     savedBandModeId);
    xml->setAttribute("filterComboId",  savedPresetComboId);
    xml->setAttribute("outputGainDB",   (double)savedOutputGainDB);

    for (int i = 0; i < savedBandCount && i < maxBands; ++i)
    {
        auto* bandXml = xml->createNewChildElement("Band");
        bandXml->setAttribute("index",      i);
        bandXml->setAttribute("frequency",  (double)savedBands[i].frequency);
        bandXml->setAttribute("gainDB",     (double)savedBands[i].gainDB);
        bandXml->setAttribute("Q",          (double)savedBands[i].Q);
        bandXml->setAttribute("filterType", savedBands[i].filterType);
    }

    // Save user presets
    for (int p = 0; p < (int)userPresets.size(); ++p)
    {
        auto* presetXml = xml->createNewChildElement("UserPreset");
        presetXml->setAttribute("name",       userPresets[p].name);
        presetXml->setAttribute("bandCount",  userPresets[p].bandCount);
        presetXml->setAttribute("bandModeId", userPresets[p].bandModeId);

        for (int i = 0; i < userPresets[p].bandCount && i < maxBands; ++i)
        {
            auto* bXml = presetXml->createNewChildElement("Band");
            bXml->setAttribute("index",      i);
            bXml->setAttribute("frequency",  (double)userPresets[p].bands[i].frequency);
            bXml->setAttribute("gainDB",     (double)userPresets[p].bands[i].gainDB);
            bXml->setAttribute("Q",          (double)userPresets[p].bands[i].Q);
            bXml->setAttribute("filterType", userPresets[p].bands[i].filterType);
        }
    }

    copyXmlToBinary(*xml, destData);
}

void OrionSoundEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml != nullptr && xml->hasTagName("OrionSoundEQState"))
    {
        savedBandCount      = xml->getIntAttribute("bandCount", 5);
        savedBandModeId     = xml->getIntAttribute("bandModeId", 1);
        savedPresetComboId  = xml->getIntAttribute("filterComboId", 7);
        savedOutputGainDB   = (float)xml->getDoubleAttribute("outputGainDB", 0.0);

        savedBandCount = juce::jlimit(1, maxBands, savedBandCount);

        for (auto* childXml : xml->getChildIterator())
        {
            if (childXml->hasTagName("Band"))
            {
                int idx = childXml->getIntAttribute("index", -1);
                if (idx >= 0 && idx < maxBands)
                {
                    savedBands[idx].frequency  = (float)childXml->getDoubleAttribute("frequency", 1000.0);
                    savedBands[idx].gainDB     = (float)childXml->getDoubleAttribute("gainDB", 0.0);
                    savedBands[idx].Q          = (float)childXml->getDoubleAttribute("Q", 1.0);
                    savedBands[idx].filterType = childXml->getIntAttribute("filterType", 0);
                }
            }
            else if (childXml->hasTagName("UserPreset"))
            {
                UserPreset up;
                up.name       = childXml->getStringAttribute("name", "Untitled");
                up.bandCount  = juce::jlimit(1, maxBands, childXml->getIntAttribute("bandCount", 8));
                up.bandModeId = childXml->getIntAttribute("bandModeId", 1);

                for (auto* bXml : childXml->getChildIterator())
                {
                    if (bXml->hasTagName("Band"))
                    {
                        int idx = bXml->getIntAttribute("index", -1);
                        if (idx >= 0 && idx < maxBands)
                        {
                            up.bands[idx].frequency  = (float)bXml->getDoubleAttribute("frequency", 1000.0);
                            up.bands[idx].gainDB     = (float)bXml->getDoubleAttribute("gainDB", 0.0);
                            up.bands[idx].Q          = (float)bXml->getDoubleAttribute("Q", 1.0);
                            up.bands[idx].filterType = bXml->getIntAttribute("filterType", 0);
                        }
                    }
                }

                userPresets.push_back(up);
            }
        }

        stateHasBeenLoaded = true;
    }
}

// createPluginFilter removed — Orion is hosted internally by ARK
