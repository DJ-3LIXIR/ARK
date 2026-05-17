/*
  ==============================================================================
    HADES - Plugin Processor
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout HadesAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Guitar
    layout.add (std::make_unique<juce::AudioParameterFloat> ("guitar_gain",     "Guitar Gain",     0.0f, 1.0f, 0.85f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("guitar_bass",     "Guitar Bass",     0.0f, 1.0f, 0.7f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("guitar_mids",     "Guitar Mids",     0.0f, 1.0f, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("guitar_treble",   "Guitar Treble",   0.0f, 1.0f, 0.75f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("guitar_presence", "Guitar Presence", 0.0f, 1.0f, 0.75f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("guitar_master",   "Guitar Master",   0.0f, 1.0f, 0.65f));
    layout.add (std::make_unique<juce::AudioParameterInt>   ("guitar_channel",  "Guitar Channel",  0, 2, 2));

    // Bass
    layout.add (std::make_unique<juce::AudioParameterFloat> ("bass_gain",   "Bass Gain",   0.0f, 1.0f, 0.8f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("bass_low",    "Bass Low",    0.0f, 1.0f, 0.75f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("bass_lomid",  "Bass LoMid",  0.0f, 1.0f, 0.35f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("bass_himid",  "Bass HiMid",  0.0f, 1.0f, 0.35f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("bass_high",   "Bass High",   0.0f, 1.0f, 0.6f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("bass_master", "Bass Master", 0.0f, 1.0f, 0.6f));

    // Cabinet
    layout.add (std::make_unique<juce::AudioParameterFloat> ("cab_mix",    "Cab Mix",     0.0f, 1.0f, 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("cab_mic",    "Cab Mic",     0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("cab_air",    "Cab Air",     0.0f, 1.0f, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("cab_lowcut", "Cab Low Cut", 20.0f, 300.0f, 80.0f));
    layout.add (std::make_unique<juce::AudioParameterInt>   ("cab_type",   "Cab Type",    0, 5, 0));
    layout.add (std::make_unique<juce::AudioParameterBool>  ("cab_bypass", "Cab Bypass",  false));

    // Global
    layout.add (std::make_unique<juce::AudioParameterInt>   ("active_amp",     "Active Amp",     0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("output_volume",  "Output Volume",  0.0f, 1.0f, 0.8f));

    return layout;
}

//==============================================================================
HadesAudioProcessor::HadesAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "HADES_STATE", createParameterLayout())
{
    airDelayBufferL.fill (0.0f);
    airDelayBufferR.fill (0.0f);
}

HadesAudioProcessor::~HadesAudioProcessor() {}

//==============================================================================
const juce::String HadesAudioProcessor::getName() const { return "Hades"; }
bool HadesAudioProcessor::acceptsMidi() const { return false; }
bool HadesAudioProcessor::producesMidi() const { return false; }
bool HadesAudioProcessor::isMidiEffect() const { return false; }
double HadesAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int HadesAudioProcessor::getNumPrograms() { return 1; }
int HadesAudioProcessor::getCurrentProgram() { return 0; }
void HadesAudioProcessor::setCurrentProgram (int) {}
const juce::String HadesAudioProcessor::getProgramName (int) { return {}; }
void HadesAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void HadesAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, 1 };

    // Prepare guitar filters (per channel so use spec with 1 channel)
    guitarBassFilter.prepare (spec);
    guitarBassFilterR.prepare (spec);
    guitarMidsFilter.prepare (spec);
    guitarMidsFilterR.prepare (spec);
    guitarTrebleFilter.prepare (spec);
    guitarTrebleFilterR.prepare (spec);
    guitarPresenceFilter.prepare (spec);
    guitarPresenceFilterR.prepare (spec);

    // Prepare bass filters
    bassLowFilter.prepare (spec);
    bassLowFilterR.prepare (spec);
    bassLoMidFilter.prepare (spec);
    bassLoMidFilterR.prepare (spec);
    bassHiMidFilter.prepare (spec);
    bassHiMidFilterR.prepare (spec);
    bassHighFilter.prepare (spec);
    bassHighFilterR.prepare (spec);

    // Prepare cabinet filters
    juce::dsp::ProcessSpec stereoSpec { sampleRate, (juce::uint32)samplesPerBlock, 2 };
    cabLowCutFilter.prepare (stereoSpec);
    cabLowCutFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);

    cabFilter1.prepare (spec);  cabFilter1R.prepare (spec);
    cabFilter2.prepare (spec);  cabFilter2R.prepare (spec);
    cabFilter3.prepare (spec);  cabFilter3R.prepare (spec);
    cabFilter4.prepare (spec);  cabFilter4R.prepare (spec);

    // Reset air delay buffer
    airDelayBufferL.fill (0.0f);
    airDelayBufferR.fill (0.0f);
    airDelayWritePos = 0;

    // Load default cab (type 0)
    updateCabinetFilters (0);

    // Set default guitar filters
    *guitarBassFilter.coefficients     = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sampleRate, 250.0f,  0.7f, 1.0f);
    *guitarBassFilterR.coefficients    = *guitarBassFilter.coefficients;
    *guitarMidsFilter.coefficients     = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 800.0f,  0.8f, 1.0f);
    *guitarMidsFilterR.coefficients    = *guitarMidsFilter.coefficients;
    *guitarTrebleFilter.coefficients   = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 3500.0f, 0.7f, 1.0f);
    *guitarTrebleFilterR.coefficients  = *guitarTrebleFilter.coefficients;
    *guitarPresenceFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 6000.0f, 0.7f, 1.0f);
    *guitarPresenceFilterR.coefficients= *guitarPresenceFilter.coefficients;

    // Set default bass filters
    *bassLowFilter.coefficients    = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sampleRate, 80.0f,   0.7f, 1.0f);
    *bassLowFilterR.coefficients   = *bassLowFilter.coefficients;
    *bassLoMidFilter.coefficients  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 300.0f,  0.8f, 1.0f);
    *bassLoMidFilterR.coefficients = *bassLoMidFilter.coefficients;
    *bassHiMidFilter.coefficients  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 1200.0f, 0.8f, 1.0f);
    *bassHiMidFilterR.coefficients = *bassHiMidFilter.coefficients;
    *bassHighFilter.coefficients   = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 5000.0f, 0.7f, 1.0f);
    *bassHighFilterR.coefficients  = *bassHighFilter.coefficients;
}

void HadesAudioProcessor::releaseResources()
{
    guitarBassFilter.reset();     guitarBassFilterR.reset();
    guitarMidsFilter.reset();     guitarMidsFilterR.reset();
    guitarTrebleFilter.reset();   guitarTrebleFilterR.reset();
    guitarPresenceFilter.reset(); guitarPresenceFilterR.reset();
    bassLowFilter.reset();        bassLowFilterR.reset();
    bassLoMidFilter.reset();      bassLoMidFilterR.reset();
    bassHiMidFilter.reset();      bassHiMidFilterR.reset();
    bassHighFilter.reset();       bassHighFilterR.reset();
    cabLowCutFilter.reset();
    cabFilter1.reset(); cabFilter1R.reset();
    cabFilter2.reset(); cabFilter2R.reset();
    cabFilter3.reset(); cabFilter3R.reset();
    cabFilter4.reset(); cabFilter4R.reset();
}

bool HadesAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

//==============================================================================
void HadesAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // active_amp selects which amp is routed (0=guitar, 1=bass).
    // Each amp's power button independently bypasses its own processing.
    int activeAmp = (int)*apvts.getRawParameterValue ("active_amp");

    if (activeAmp == 0)
    {
        if (guitarActive.load())
            processGuitar (buffer);
        else
            buffer.clear();
    }
    else
    {
        if (bassActive.load())
            processBass (buffer);
        else
            buffer.clear();
    }

    bool cabBypass = (bool)*apvts.getRawParameterValue ("cab_bypass");
    if (!cabBypass && cabActive.load())
        processCabinet (buffer);

    // Apply output volume
    float outVol = *apvts.getRawParameterValue ("output_volume");
    buffer.applyGain (outVol);

    // RMS metering
    float rms = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        rms += buffer.getRMSLevel (ch, 0, buffer.getNumSamples());
    rms /= (float)buffer.getNumChannels();
    rmsLevel.store (rms);
}

//==============================================================================
void HadesAudioProcessor::processGuitar (juce::AudioBuffer<float>& buffer)
{
    auto* dataL = buffer.getWritePointer (0);
    auto* dataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : dataL;
    int   numSamples = buffer.getNumSamples();

    float gain     = *apvts.getRawParameterValue ("guitar_gain");
    float bass     = *apvts.getRawParameterValue ("guitar_bass");
    float mids     = *apvts.getRawParameterValue ("guitar_mids");
    float treble   = *apvts.getRawParameterValue ("guitar_treble");
    float presence = *apvts.getRawParameterValue ("guitar_presence");
    float master   = *apvts.getRawParameterValue ("guitar_master");
    int   channel  = (int)*apvts.getRawParameterValue ("guitar_channel");

    // Update tone stack filters
    float bassGain     = juce::Decibels::decibelsToGain (juce::jmap (bass,     0.0f, 1.0f, -12.0f, 12.0f));
    float midsGain     = juce::Decibels::decibelsToGain (juce::jmap (mids,     0.0f, 1.0f, -12.0f, 12.0f));
    float trebleGain   = juce::Decibels::decibelsToGain (juce::jmap (treble,   0.0f, 1.0f, -12.0f, 12.0f));
    float presenceGain = juce::Decibels::decibelsToGain (juce::jmap (presence, 0.0f, 1.0f, -6.0f,  12.0f));

    *guitarBassFilter.coefficients     = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (currentSampleRate, 250.0f,  0.7f, bassGain);
    *guitarBassFilterR.coefficients    = *guitarBassFilter.coefficients;
    *guitarMidsFilter.coefficients     = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (currentSampleRate, 800.0f,  0.8f, midsGain);
    *guitarMidsFilterR.coefficients    = *guitarMidsFilter.coefficients;
    *guitarTrebleFilter.coefficients   = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 3500.0f, 0.7f, trebleGain);
    *guitarTrebleFilterR.coefficients  = *guitarTrebleFilter.coefficients;
    *guitarPresenceFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 6000.0f, 0.7f, presenceGain);
    *guitarPresenceFilterR.coefficients= *guitarPresenceFilter.coefficients;

    // Drive amount per channel mode
    float driveAmount = 1.0f;
    if      (channel == 0) driveAmount = 2.0f  + gain * 4.0f;   // Clean
    else if (channel == 1) driveAmount = 6.0f  + gain * 12.0f;  // Crunch
    else                   driveAmount = 15.0f + gain * 35.0f;  // Lead

    for (int i = 0; i < numSamples; ++i)
    {
        // Left channel
        float sL = dataL[i] * driveAmount;
        // Soft clip (tanh saturation)
        sL = std::tanh (sL);
        // Tone stack
        sL = guitarBassFilter.processSample    (sL);
        sL = guitarMidsFilter.processSample    (sL);
        sL = guitarTrebleFilter.processSample  (sL);
        sL = guitarPresenceFilter.processSample(sL);
        dataL[i] = sL * master;

        // Right channel
        float sR = dataR[i] * driveAmount;
        sR = std::tanh (sR);
        sR = guitarBassFilterR.processSample    (sR);
        sR = guitarMidsFilterR.processSample    (sR);
        sR = guitarTrebleFilterR.processSample  (sR);
        sR = guitarPresenceFilterR.processSample(sR);
        dataR[i] = sR * master;
    }
}

//==============================================================================
void HadesAudioProcessor::processBass (juce::AudioBuffer<float>& buffer)
{
    auto* dataL = buffer.getWritePointer (0);
    auto* dataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : dataL;
    int   numSamples = buffer.getNumSamples();

    float gain   = *apvts.getRawParameterValue ("bass_gain");
    float low    = *apvts.getRawParameterValue ("bass_low");
    float lomid  = *apvts.getRawParameterValue ("bass_lomid");
    float himid  = *apvts.getRawParameterValue ("bass_himid");
    float high   = *apvts.getRawParameterValue ("bass_high");
    float master = *apvts.getRawParameterValue ("bass_master");

    float lowGain   = juce::Decibels::decibelsToGain (juce::jmap (low,   0.0f, 1.0f, -12.0f, 12.0f));
    float lomidGain = juce::Decibels::decibelsToGain (juce::jmap (lomid, 0.0f, 1.0f, -12.0f, 12.0f));
    float himidGain = juce::Decibels::decibelsToGain (juce::jmap (himid, 0.0f, 1.0f, -12.0f, 12.0f));
    float highGain  = juce::Decibels::decibelsToGain (juce::jmap (high,  0.0f, 1.0f, -12.0f, 12.0f));

    *bassLowFilter.coefficients    = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (currentSampleRate, 80.0f,   0.7f, lowGain);
    *bassLowFilterR.coefficients   = *bassLowFilter.coefficients;
    *bassLoMidFilter.coefficients  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (currentSampleRate, 300.0f,  0.8f, lomidGain);
    *bassLoMidFilterR.coefficients = *bassLoMidFilter.coefficients;
    *bassHiMidFilter.coefficients  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (currentSampleRate, 1200.0f, 0.8f, himidGain);
    *bassHiMidFilterR.coefficients = *bassHiMidFilter.coefficients;
    *bassHighFilter.coefficients   = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 5000.0f, 0.7f, highGain);
    *bassHighFilterR.coefficients  = *bassHighFilter.coefficients;

    // Softer saturation for bass — less aggressive than guitar
    float driveAmount = 1.5f + gain * 8.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float sL = dataL[i] * driveAmount;
        sL = std::tanh (sL * 0.7f) / 0.7f; // Softer clip
        sL = bassLowFilter.processSample   (sL);
        sL = bassLoMidFilter.processSample (sL);
        sL = bassHiMidFilter.processSample (sL);
        sL = bassHighFilter.processSample  (sL);
        dataL[i] = sL * master;

        float sR = dataR[i] * driveAmount;
        sR = std::tanh (sR * 0.7f) / 0.7f;
        sR = bassLowFilterR.processSample   (sR);
        sR = bassLoMidFilterR.processSample (sR);
        sR = bassHiMidFilterR.processSample (sR);
        sR = bassHighFilterR.processSample  (sR);
        dataR[i] = sR * master;
    }
}

//==============================================================================
void HadesAudioProcessor::processCabinet (juce::AudioBuffer<float>& buffer)
{
    auto* dataL = buffer.getWritePointer (0);
    auto* dataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : dataL;
    int   numSamples = buffer.getNumSamples();

    float mix    = *apvts.getRawParameterValue ("cab_mix");
    float air    = *apvts.getRawParameterValue ("cab_air");
    float lowcut = *apvts.getRawParameterValue ("cab_lowcut");
    int   cabType= (int)*apvts.getRawParameterValue ("cab_type");

    // Update low cut
    cabLowCutFilter.setCutoffFrequency (lowcut);

    // Update cab filters if needed
    updateCabinetFilters (cabType);

    for (int i = 0; i < numSamples; ++i)
    {
        float dryL = dataL[i];
        float dryR = dataR[i];

        // Low cut
        float wetL = cabLowCutFilter.processSample (0, dryL);
        float wetR = cabLowCutFilter.processSample (1, dryR);

        // Cab colour filters
        wetL = cabFilter1.processSample (wetL);
        wetL = cabFilter2.processSample (wetL);
        wetL = cabFilter3.processSample (wetL);
        wetL = cabFilter4.processSample (wetL);

        wetR = cabFilter1R.processSample (wetR);
        wetR = cabFilter2R.processSample (wetR);
        wetR = cabFilter3R.processSample (wetR);
        wetR = cabFilter4R.processSample (wetR);

        // Air — simple feedback comb for subtle room feel
        int delayMs  = 12;
        int delaySamples = (int)(currentSampleRate * delayMs / 1000.0);
        int readPos  = (airDelayWritePos - delaySamples + (int)airDelayBufferL.size()) % (int)airDelayBufferL.size();
        float airL   = airDelayBufferL[readPos] * air * 0.4f;
        float airR   = airDelayBufferR[readPos] * air * 0.4f;
        airDelayBufferL[airDelayWritePos] = wetL + airL * 0.3f;
        airDelayBufferR[airDelayWritePos] = wetR + airR * 0.3f;
        airDelayWritePos = (airDelayWritePos + 1) % (int)airDelayBufferL.size();
        wetL += airL;
        wetR += airR;

        // Mix
        dataL[i] = dryL * (1.0f - mix) + wetL * mix;
        dataR[i] = dryR * (1.0f - mix) + wetR * mix;
    }
}

//==============================================================================
void HadesAudioProcessor::updateCabinetFilters (int cabType)
{
    // Each cab type is approximated with 4 biquad filters
    // [lowShelf, peak1, peak2, highShelf]
    struct CabEQ { float lsFreq, lsGain, p1Freq, p1Gain, p2Freq, p2Gain, hsFreq, hsGain; };

    const CabEQ cabs[] = {
        { 120.0f,  3.0f,  800.0f, -4.0f, 3000.0f, -2.0f, 5000.0f, -8.0f  }, // 4x12 Vintage
        { 100.0f,  1.0f,  700.0f, -6.0f, 3500.0f,  1.0f, 5500.0f, -6.0f  }, // 4x12 Modern
        { 80.0f,  -2.0f,  900.0f,  2.0f, 4000.0f,  3.0f, 6000.0f, -4.0f  }, // 2x12 Open Back
        { 100.0f,  0.0f, 1000.0f,  3.0f, 3500.0f,  2.0f, 5000.0f, -5.0f  }, // 1x12 Combo
        { 110.0f,  2.0f,  750.0f, -3.0f, 3000.0f, -1.0f, 5000.0f, -7.0f  }, // 2x12 Closed
        { 60.0f,   6.0f,  400.0f,  2.0f, 2000.0f, -2.0f, 4000.0f, -10.0f }, // 8x10 Bass
    };

    int idx = juce::jlimit (0, 5, cabType);
    auto& c = cabs[idx];

    auto lsG  = juce::Decibels::decibelsToGain (c.lsGain);
    auto p1G  = juce::Decibels::decibelsToGain (c.p1Gain);
    auto p2G  = juce::Decibels::decibelsToGain (c.p2Gain);
    auto hsG  = juce::Decibels::decibelsToGain (c.hsGain);

    *cabFilter1.coefficients  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (currentSampleRate, c.lsFreq, 0.7f, lsG);
    *cabFilter1R.coefficients = *cabFilter1.coefficients;
    *cabFilter2.coefficients  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (currentSampleRate, c.p1Freq, 0.8f, p1G);
    *cabFilter2R.coefficients = *cabFilter2.coefficients;
    *cabFilter3.coefficients  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (currentSampleRate, c.p2Freq, 0.8f, p2G);
    *cabFilter3R.coefficients = *cabFilter3.coefficients;
    *cabFilter4.coefficients  = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, c.hsFreq, 0.7f, hsG);
    *cabFilter4R.coefficients = *cabFilter4.coefficients;
}

//==============================================================================
void HadesAudioProcessor::setParameterValue (const juce::String& paramId, float value)
{
    if (auto* param = apvts.getParameter (paramId))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (value));
        param->endChangeGesture();
    }
}

//==============================================================================
void HadesAudioProcessor::loadGuitarPreset (int presetIndex)
{
    struct GuitarPreset { float gain, bass, mids, treble, presence, master; int channel; };
    const GuitarPreset presets[] = {
        { 0.75f, 0.7f,  0.48f, 0.75f, 0.65f, 0.65f, 2 }, // HADES
        { 0.15f, 0.5f,  0.55f, 0.5f,  0.4f,  0.7f,  0 }, // Clean
        { 0.45f, 0.55f, 0.5f,  0.55f, 0.5f,  0.65f, 1 }, // Crunch
        { 0.75f, 0.6f,  0.35f, 0.65f, 0.6f,  0.6f,  2 }, // High Gain
        { 0.9f,  0.65f, 0.25f, 0.7f,  0.65f, 0.55f, 2 }, // Metal
        { 0.7f,  0.45f, 0.65f, 0.6f,  0.7f,  0.6f,  2 }, // Lead
    };

    int idx = juce::jlimit (0, 5, presetIndex);
    auto& p = presets[idx];
    setParameterValue ("guitar_gain",     p.gain);
    setParameterValue ("guitar_bass",     p.bass);
    setParameterValue ("guitar_mids",     p.mids);
    setParameterValue ("guitar_treble",   p.treble);
    setParameterValue ("guitar_presence", p.presence);
    setParameterValue ("guitar_master",   p.master);
    if (auto* param = apvts.getParameter ("guitar_channel"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float)p.channel));
        param->endChangeGesture();
    }
}

//==============================================================================
void HadesAudioProcessor::loadBassPreset (int presetIndex)
{
    struct BassPreset { float gain, low, lomid, himid, high, master; };
    const BassPreset presets[] = {
        { 0.8f,  0.75f, 0.35f, 0.35f, 0.6f,  0.6f  }, // HADES
        { 0.3f,  0.65f, 0.5f,  0.4f,  0.35f, 0.7f  }, // Warm
        { 0.5f,  0.55f, 0.6f,  0.55f, 0.45f, 0.65f }, // Punchy
        { 0.65f, 0.6f,  0.5f,  0.5f,  0.5f,  0.6f  }, // Gritty
        { 0.82f, 0.65f, 0.45f, 0.45f, 0.55f, 0.55f }, // Distorted
        { 0.1f,  0.5f,  0.5f,  0.5f,  0.5f,  0.75f }, // Clean DI
    };

    int idx = juce::jlimit (0, 5, presetIndex);
    auto& p = presets[idx];
    setParameterValue ("bass_gain",   p.gain);
    setParameterValue ("bass_low",    p.low);
    setParameterValue ("bass_lomid",  p.lomid);
    setParameterValue ("bass_himid",  p.himid);
    setParameterValue ("bass_high",   p.high);
    setParameterValue ("bass_master", p.master);
}

//==============================================================================
void HadesAudioProcessor::loadCabinetPreset (int presetIndex)
{
    struct CabPreset { int type; float mic, air, lowcut, mix; };
    const CabPreset presets[] = {
        { 0, 0.4f, 0.3f,  80.0f,  1.0f }, // 4x12 Vintage
        { 1, 0.5f, 0.2f,  100.0f, 1.0f }, // 4x12 Modern
        { 2, 0.6f, 0.5f,  60.0f,  1.0f }, // 2x12 Open Back
        { 3, 0.5f, 0.4f,  70.0f,  1.0f }, // 1x12 Combo
        { 4, 0.45f,0.2f,  90.0f,  1.0f }, // 2x12 Closed
        { 5, 0.4f, 0.25f, 40.0f,  1.0f }, // 8x10 Bass
    };

    int idx = juce::jlimit (0, 5, presetIndex);
    auto& p = presets[idx];
    if (auto* param = apvts.getParameter ("cab_type"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float)p.type));
        param->endChangeGesture();
    }
    setParameterValue ("cab_mic",    p.mic);
    setParameterValue ("cab_air",    p.air);
    setParameterValue ("cab_lowcut", p.lowcut);
    setParameterValue ("cab_mix",    p.mix);
}

//==============================================================================
bool HadesAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* HadesAudioProcessor::createEditor()
{
    return new HadesAudioProcessorEditor (*this);
}

//==============================================================================
void HadesAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("guitarPreset",  savedPresetIndex[0]);
    xml->setAttribute ("bassPreset",    savedPresetIndex[1]);
    xml->setAttribute ("cabinetPreset", savedPresetIndex[2]);
    xml->setAttribute ("guitarActive",  (int)guitarActive.load());
    xml->setAttribute ("bassActive",    (int)bassActive.load());
    xml->setAttribute ("cabActive",     (int)cabActive.load());
    xml->setAttribute ("pageIndex",     savedPageIndex);
    copyXmlToBinary (*xml, destData);
}

void HadesAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        savedPresetIndex[0] = xml->getIntAttribute ("guitarPreset",  0);
        savedPresetIndex[1] = xml->getIntAttribute ("bassPreset",    0);
        savedPresetIndex[2] = xml->getIntAttribute ("cabinetPreset", 0);
        guitarActive.store ((bool)xml->getIntAttribute ("guitarActive", 1));
        bassActive.store   ((bool)xml->getIntAttribute ("bassActive",   1));
        cabActive.store    ((bool)xml->getIntAttribute ("cabActive",    1));
        savedPageIndex = xml->getIntAttribute ("pageIndex", 0);
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

// createPluginFilter removed — Hades is hosted internally by ARK
