/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
APOLLOAudioProcessor::APOLLOAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache all raw parameter pointers for efficient per-block access
    attackParam      = apvts.getRawParameterValue ("attack");
    sizeParam        = apvts.getRawParameterValue ("size");
    densityParam     = apvts.getRawParameterValue ("density");
    decayParam       = apvts.getRawParameterValue ("decay");
    distanceParam    = apvts.getRawParameterValue ("distance");
    predelayParam    = apvts.getRawParameterValue ("predelay");
    dryParam         = apvts.getRawParameterValue ("dry");
    wetParam         = apvts.getRawParameterValue ("wet");
    freezeParam      = apvts.getRawParameterValue ("freeze");
    reverbOnParam    = apvts.getRawParameterValue ("reverbOn");
    roomTypeParam    = apvts.getRawParameterValue ("roomType");
    qualityParam     = apvts.getRawParameterValue ("quality");
    modSpeedParam    = apvts.getRawParameterValue ("modSpeed");
    modDepthParam    = apvts.getRawParameterValue ("modDepth");
    modSourceParam   = apvts.getRawParameterValue ("modSource");
    smoothingParam   = apvts.getRawParameterValue ("smoothing");
    earlyLateParam   = apvts.getRawParameterValue ("earlyLate");
    widthParam       = apvts.getRawParameterValue ("width");
    monoMakerParam   = apvts.getRawParameterValue ("monoMaker");
    monoMakerOnParam = apvts.getRawParameterValue ("monoMakerOn");
    outputEqOnParam  = apvts.getRawParameterValue ("outputEqOn");

    for (int i = 0; i < 8; ++i)
    {
        eqBandOnParams[i]   = apvts.getRawParameterValue ("eqBand" + juce::String (i + 1) + "On");
        eqBandGainParams[i] = apvts.getRawParameterValue ("eqBand" + juce::String (i + 1) + "Gain");
        eqBandQParams[i]    = apvts.getRawParameterValue ("eqBand" + juce::String (i + 1) + "Q");
        eqBandFreqParams[i] = apvts.getRawParameterValue ("eqBand" + juce::String (i + 1) + "Freq");
    }

    // Room Sim parameters
    rsOnParam         = apvts.getRawParameterValue ("rsOn");
    rsInputGainParam  = apvts.getRawParameterValue ("rsInputGain");
    rsOutputGainParam = apvts.getRawParameterValue ("rsOutputGain");
    rsRoomSizeParam   = apvts.getRawParameterValue ("rsRoomSize");
    rsAbsorptionParam = apvts.getRawParameterValue ("rsAbsorption");
    rsDiffusionParam  = apvts.getRawParameterValue ("rsDiffusion");
    rsPredelayParam   = apvts.getRawParameterValue ("rsPredelay");
    rsDecayParam      = apvts.getRawParameterValue ("rsDecay");
    rsWidthParam      = apvts.getRawParameterValue ("rsWidth");
    rsEarlyLateParam  = apvts.getRawParameterValue ("rsEarlyLate");
    rsDryParam        = apvts.getRawParameterValue ("rsDry");
    rsWetParam        = apvts.getRawParameterValue ("rsWet");
    rsRoomTypeParam   = apvts.getRawParameterValue ("rsRoomType");
    rsPhaseParam      = apvts.getRawParameterValue ("rsPhase");
    rsMonoParam       = apvts.getRawParameterValue ("rsMono");
    rsLockParam       = apvts.getRawParameterValue ("rsLock");
}

juce::AudioProcessorValueTreeState::ParameterLayout APOLLOAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("attack", 1), "Attack",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("size", 1), "Size",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("density", 1), "Density",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("decay", 1), "Decay",
        juce::NormalisableRange<float> (0.3f, 100.0f, 0.01f, 0.3f), 1.1f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("distance", 1), "Distance",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("predelay", 1), "Predelay",
        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), 8.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("dry", 1), "Dry",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("wet", 1), "Wet",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("freeze", 1), "Freeze", false));

    // --- DETAILS page parameters ---
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("quality", 1), "Quality",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("modSpeed", 1), "Mod Speed",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f, 0.4f), 0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("modDepth", 1), "Mod Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("smoothing", 1), "Smoothing",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("earlyLate", 1), "Early/Late",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("width", 1), "Width",
        juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("monoMaker", 1), "Mono Maker",
        juce::NormalisableRange<float> (0.0f, 500.0f, 1.0f, 0.4f), 0.0f));

    // --- Output EQ master enable ---
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("outputEqOn", 1), "Output EQ On", true));

    // --- Output EQ band enable parameters ---
    for (int i = 1; i <= 8; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID ("eqBand" + juce::String (i) + "On", 1),
            "EQ Band " + juce::String (i) + " On", true));
    }

    // --- Modulation source waveform ---
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("modSource", 1), "Mod Source",
        juce::StringArray { "Sine", "Square", "Noise" },
        0));

    // --- Reverb master enable ---
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("reverbOn", 1), "Reverb On", true));

    // --- Room type selection ---
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("roomType", 1), "Room Type",
        juce::StringArray { "Room", "Chamber", "Concert Hall", "Theatre",
                            "Synth Hall", "Digital", "Dark Room", "Dense",
                            "Smooth Space", "Vocal Hall", "Reflective Hall",
                            "Strange Room", "Airy", "Bloomy" },
        0));

    // --- Decay mode (Time / Note) ---
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("decayMode", 1), "Decay Mode",
        juce::StringArray { "Time", "Note" }, 0));

    // --- Predelay mode (Time / Note) ---
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("predelayMode", 1), "Predelay Mode",
        juce::StringArray { "Time", "Note" }, 0));

    // --- Mono Maker enable ---
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("monoMakerOn", 1), "Mono Maker On", true));

    // --- Output EQ band gain parameters (dB) ---
    for (int i = 1; i <= 8; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("eqBand" + juce::String (i) + "Gain", 1),
            "EQ Band " + juce::String (i) + " Gain",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    }

    // --- Output EQ band Q parameters ---
    for (int i = 1; i <= 8; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("eqBand" + juce::String (i) + "Q", 1),
            "EQ Band " + juce::String (i) + " Q",
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 0.707f));
    }

    // --- Damping EQ dot frequency positions (visual-only, no DSP) ---
    const float dampDotDefaults[] = { 60.0f, 250.0f, 1000.0f, 4000.0f, 12000.0f };
    for (int i = 1; i <= 5; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("dampDotFreq" + juce::String (i), 1),
            "Damp Dot " + juce::String (i) + " Freq",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f),
            dampDotDefaults[i - 1]));
    }

    // --- Output EQ band frequency parameters (Hz, logarithmic range) ---
    // Default frequencies match OutputEQ::bandFreqs: 40, 100, 250, 800, 2500, 6000, 10000, 16000
    const float eqDefaultFreqs[] = { 40.0f, 100.0f, 250.0f, 800.0f, 2500.0f, 6000.0f, 10000.0f, 16000.0f };
    for (int i = 1; i <= 8; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("eqBand" + juce::String (i) + "Freq", 1),
            "EQ Band " + juce::String (i) + " Freq",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), eqDefaultFreqs[i - 1]));
    }

    // =================================================================
    // ROOM SIMULATOR parameters
    // =================================================================
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("rsOn", 1), "Room Sim On", true));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsInputGain", 1), "RS Input Gain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsOutputGain", 1), "RS Output Gain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsRoomSize", 1), "RS Room Size",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f), 40.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsAbsorption", 1), "RS Absorption",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsDiffusion", 1), "RS Diffusion",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 65.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsPredelay", 1), "RS Pre-Delay",
        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), 12.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsDecay", 1), "RS Decay",
        juce::NormalisableRange<float> (0.1f, 30.0f, 0.01f, 0.3f), 2.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsWidth", 1), "RS Width",
        juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsEarlyLate", 1), "RS Early/Late",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsDry", 1), "RS Dry",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("rsWet", 1), "RS Wet",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("rsRoomType", 1), "RS Room Type",
        juce::StringArray { "Theater", "Church", "Studio", "Hall",
                            "Garage", "Bathroom", "Bedroom", "Warehouse" }, 0));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("rsPhase", 1), "RS Phase Invert", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("rsMono", 1), "RS Mono", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("rsLock", 1), "RS Lock", false));

    return layout;
}

APOLLOAudioProcessor::~APOLLOAudioProcessor()
{
}

//==============================================================================
const juce::String APOLLOAudioProcessor::getName() const
{
    return "Apollo";
}

bool APOLLOAudioProcessor::acceptsMidi() const { return false; }
bool APOLLOAudioProcessor::producesMidi() const { return false; }
bool APOLLOAudioProcessor::isMidiEffect() const { return false; }

double APOLLOAudioProcessor::getTailLengthSeconds() const
{
    return 100.0; // max decay time
}

int APOLLOAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int APOLLOAudioProcessor::getCurrentProgram()
{
    return 0;
}

void APOLLOAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String APOLLOAudioProcessor::getProgramName (int index)
{
    return {};
}

void APOLLOAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void APOLLOAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    reverbEngine.prepare (sampleRate, samplesPerBlock);
    roomSimEngine.prepare (sampleRate, samplesPerBlock);
}

void APOLLOAudioProcessor::releaseResources()
{
    reverbEngine.reset();
    roomSimEngine.reset();
}

bool APOLLOAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void APOLLOAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();

    // ---- Read all reverb parameters ----
    bool eqBandOn[8];
    float eqBandGain[8], eqBandQ[8];
    for (int i = 0; i < 8; ++i)
    {
        eqBandOn[i]   = eqBandOnParams[i]->load() > 0.5f;
        eqBandGain[i] = eqBandGainParams[i]->load();
        eqBandQ[i]    = eqBandQParams[i]->load();
    }

    reverbEngine.updateParameters (
        attackParam->load(),
        sizeParam->load(),
        densityParam->load(),
        decayParam->load(),
        distanceParam->load(),
        predelayParam->load(),
        dryParam->load(),
        wetParam->load(),
        freezeParam->load() > 0.5f,
        reverbOnParam->load() > 0.5f,
        (int) roomTypeParam->load(),
        qualityParam->load(),
        modSpeedParam->load(),
        modDepthParam->load(),
        (int) modSourceParam->load(),
        smoothingParam->load(),
        earlyLateParam->load(),
        widthParam->load(),
        monoMakerParam->load(),
        monoMakerOnParam->load() > 0.5f,
        outputEqOnParam->load() > 0.5f,
        eqBandOn, eqBandGain, eqBandQ
    );

    reverbEngine.processBlock (buffer, numSamples);

    // ---- Room Sim (processes after reverb in series) ----
    roomSimEngine.updateParameters (
        rsOnParam->load() > 0.5f,
        rsInputGainParam->load(),
        rsOutputGainParam->load(),
        rsRoomSizeParam->load(),
        rsAbsorptionParam->load(),
        rsDiffusionParam->load(),
        rsPredelayParam->load(),
        rsDecayParam->load(),
        rsWidthParam->load(),
        rsEarlyLateParam->load(),
        rsDryParam->load(),
        rsWetParam->load(),
        (int) rsRoomTypeParam->load(),
        rsPhaseParam->load() > 0.5f,
        rsMonoParam->load() > 0.5f,
        rsLockParam->load() > 0.5f
    );

    roomSimEngine.processBlock (buffer, numSamples);

    // Compute peak level of the output across all channels
    float peakLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getReadPointer (ch);
        for (int s = 0; s < numSamples; ++s)
            peakLevel = juce::jmax (peakLevel, std::abs (data[s]));
    }

    // Smooth the level with a simple one-pole filter
    float prev = wetLevelAtomic.load (std::memory_order_relaxed);
    float smoothed = juce::jmax (peakLevel, prev * 0.85f);
    wetLevelAtomic.store (smoothed, std::memory_order_relaxed);
}

//==============================================================================
bool APOLLOAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* APOLLOAudioProcessor::createEditor()
{
    return new APOLLOAudioProcessorEditor (*this);
}

//==============================================================================
void APOLLOAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("reverbPresetIndex", savedPresetIndex[0]);
    xml->setAttribute ("roomSimPresetIndex", savedPresetIndex[1]);
    xml->setAttribute ("pageIndex", savedPageIndex);
    copyXmlToBinary (*xml, destData);
}

void APOLLOAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        savedPresetIndex[0] = xml->getIntAttribute ("reverbPresetIndex", 0);
        savedPresetIndex[1] = xml->getIntAttribute ("roomSimPresetIndex", 0);
        savedPageIndex = xml->getIntAttribute ("pageIndex", 0);
    }
}

//==============================================================================
// Preset loading

void APOLLOAudioProcessor::loadReverbPreset (int category, int subPreset)
{
    struct PresetData {
        float attack, size, density, decay, distance, predelay, dry, wet;
        int roomType;
    };

    // 7 categories, up to 6 presets each
    //                                     atk    size   dens   decay  dist   predel dry    wet    roomType
    static const PresetData presets[7][6] = {
        // 0: Small Rooms (Room=0, Chamber=1, Dark Room=6)
        {
            /* Tight Drum Room  */ {  0.0f, 18.0f, 35.0f,  0.5f, 20.0f,  2.0f, 100.0f, 35.0f, 0 },
            /* Warm Vocal Booth */ {  3.0f, 22.0f, 50.0f,  0.7f, 30.0f,  5.0f,  95.0f, 40.0f, 1 },
            /* Bright Chamber   */ {  0.0f, 30.0f, 55.0f,  0.9f, 35.0f,  8.0f,  90.0f, 45.0f, 1 },
            /* Dark Closet      */ {  2.0f, 15.0f, 40.0f,  0.6f, 15.0f,  3.0f, 100.0f, 30.0f, 6 },
            /* Live Room        */ {  1.0f, 35.0f, 45.0f,  1.0f, 40.0f, 10.0f,  85.0f, 45.0f, 0 },
            /* Snare Room       */ {  0.0f, 20.0f, 30.0f,  0.4f, 25.0f,  1.0f, 100.0f, 30.0f, 0 },
        },
        // 1: Halls (Concert Hall=2, Vocal Hall=9, Reflective Hall=10)
        {
            /* Concert Grand    */ { 10.0f, 85.0f, 70.0f,  3.5f, 80.0f, 40.0f,  75.0f, 60.0f, 2 },
            /* Vocal Hall       */ {  5.0f, 60.0f, 55.0f,  2.0f, 55.0f, 18.0f,  85.0f, 50.0f, 9 },
            /* Orchestral       */ { 12.0f, 90.0f, 75.0f,  4.0f, 85.0f, 45.0f,  70.0f, 65.0f, 2 },
            /* Bright Hall      */ {  3.0f, 70.0f, 60.0f,  2.5f, 65.0f, 20.0f,  80.0f, 55.0f, 10 },
            /* Warm Hall        */ {  8.0f, 75.0f, 65.0f,  2.8f, 70.0f, 25.0f,  80.0f, 55.0f, 9 },
            /* Recital Hall     */ {  6.0f, 55.0f, 50.0f,  1.8f, 50.0f, 15.0f,  85.0f, 45.0f, 2 },
        },
        // 2: Theaters (Theatre=3, Smooth Space=8)
        {
            /* Classic Theater  */ {  8.0f, 70.0f, 60.0f,  2.5f, 65.0f, 22.0f,  80.0f, 55.0f, 3 },
            /* Cinematic        */ { 15.0f, 80.0f, 70.0f,  3.5f, 75.0f, 35.0f,  70.0f, 65.0f, 8 },
            /* Film Score       */ { 12.0f, 85.0f, 75.0f,  4.0f, 80.0f, 40.0f,  65.0f, 70.0f, 3 },
            /* Dialogue Stage   */ {  3.0f, 50.0f, 45.0f,  1.5f, 45.0f, 12.0f,  90.0f, 40.0f, 3 },
            /* Drama Hall       */ { 10.0f, 75.0f, 65.0f,  3.0f, 70.0f, 28.0f,  75.0f, 60.0f, 8 },
            /* (unused)         */ {  0.0f,  0.0f,  0.0f,  0.3f,  0.0f,  0.0f,   0.0f,  0.0f, 0 },
        },
        // 3: Plates & Digital (Digital=5, Dense=7)
        {
            /* Bright Plate     */ {  0.0f, 40.0f, 60.0f,  1.2f, 35.0f,  5.0f,  95.0f, 40.0f, 5 },
            /* Warm Plate       */ {  2.0f, 45.0f, 55.0f,  1.5f, 40.0f,  8.0f,  90.0f, 45.0f, 7 },
            /* Vocal Plate      */ {  1.0f, 38.0f, 50.0f,  1.3f, 30.0f,  6.0f,  92.0f, 42.0f, 5 },
            /* Drum Plate       */ {  0.0f, 30.0f, 45.0f,  0.8f, 25.0f,  3.0f, 100.0f, 30.0f, 5 },
            /* Thick Plate      */ {  3.0f, 55.0f, 70.0f,  2.0f, 50.0f, 12.0f,  85.0f, 50.0f, 7 },
            /* Shimmer          */ {  5.0f, 65.0f, 80.0f,  2.5f, 60.0f, 18.0f,  75.0f, 60.0f, 5 },
        },
        // 4: Creative (Synth Hall=4, Strange Room=11, Airy=12, Bloomy=13)
        {
            /* Synth Wash       */ { 15.0f, 80.0f, 75.0f,  4.0f, 70.0f, 30.0f,  65.0f, 70.0f, 4 },
            /* Ethereal         */ { 20.0f, 90.0f, 85.0f,  6.0f, 85.0f, 45.0f,  55.0f, 75.0f, 12 },
            /* Strange Space    */ {  5.0f, 60.0f, 65.0f,  2.5f, 55.0f, 15.0f,  80.0f, 55.0f, 11 },
            /* Blooming Pad     */ { 25.0f, 85.0f, 80.0f,  5.0f, 80.0f, 40.0f,  60.0f, 72.0f, 13 },
            /* Glitch Verb      */ {  0.0f, 45.0f, 90.0f,  1.8f, 40.0f,  8.0f,  85.0f, 50.0f, 11 },
            /* Airy Texture     */ { 18.0f, 75.0f, 70.0f,  3.5f, 75.0f, 35.0f,  70.0f, 65.0f, 12 },
        },
        // 5: Vocals (Chamber=1, Vocal Hall=9, Room=0)
        {
            /* Intimate Vocal   */ {  2.0f, 25.0f, 45.0f,  0.8f, 28.0f,  6.0f,  95.0f, 35.0f, 0 },
            /* Pop Vocal        */ {  1.0f, 35.0f, 50.0f,  1.2f, 35.0f, 10.0f,  90.0f, 40.0f, 1 },
            /* Ballad Vocal     */ {  5.0f, 55.0f, 55.0f,  2.0f, 50.0f, 18.0f,  82.0f, 50.0f, 9 },
            /* Radio Vocal      */ {  0.0f, 20.0f, 40.0f,  0.6f, 22.0f,  4.0f, 100.0f, 28.0f, 0 },
            /* Choir            */ {  8.0f, 70.0f, 65.0f,  3.0f, 65.0f, 25.0f,  75.0f, 60.0f, 9 },
            /* (unused)         */ {  0.0f,  0.0f,  0.0f,  0.3f,  0.0f,  0.0f,   0.0f,  0.0f, 0 },
        },
        // 6: Ambient/FX (Synth Hall=4, Smooth Space=8, Airy=12, Bloomy=13)
        {
            /* Infinite Space   */ { 30.0f, 100.0f, 90.0f, 10.0f, 95.0f, 80.0f,  50.0f, 80.0f, 4 },
            /* Frozen Air       */ { 25.0f,  95.0f, 85.0f,  8.0f, 90.0f, 60.0f,  55.0f, 78.0f, 12 },
            /* Dark Atmosphere  */ { 20.0f,  88.0f, 78.0f,  6.0f, 82.0f, 50.0f,  60.0f, 72.0f, 13 },
            /* Cosmic Delay     */ { 15.0f,  75.0f, 70.0f,  5.0f, 70.0f, 45.0f,  65.0f, 68.0f, 8 },
            /* Drone Pad        */ { 35.0f,  98.0f, 92.0f, 12.0f, 98.0f, 90.0f,  40.0f, 85.0f, 4 },
            /* Ocean Cave       */ { 18.0f,  82.0f, 72.0f,  4.5f, 78.0f, 38.0f,  68.0f, 65.0f, 8 },
        },
    };

    static const int presetsPerCategory[7] = { 6, 6, 5, 6, 6, 5, 6 };

    if (category < 0 || category >= 7 || subPreset < 0 || subPreset >= presetsPerCategory[category])
        return;

    const PresetData& p = presets[category][subPreset];

    apvts.getParameter ("attack")->setValueNotifyingHost (p.attack / 100.0f);
    apvts.getParameter ("size")->setValueNotifyingHost (p.size / 100.0f);
    apvts.getParameter ("density")->setValueNotifyingHost (p.density / 100.0f);
    apvts.getParameter ("decay")->setValueNotifyingHost (juce::jlimit (0.3f, 100.0f, p.decay) / 100.0f);
    apvts.getParameter ("distance")->setValueNotifyingHost (p.distance / 100.0f);
    apvts.getParameter ("predelay")->setValueNotifyingHost (p.predelay / 200.0f);
    apvts.getParameter ("dry")->setValueNotifyingHost (p.dry / 100.0f);
    apvts.getParameter ("wet")->setValueNotifyingHost (p.wet / 100.0f);
    apvts.getParameter ("roomType")->setValueNotifyingHost (p.roomType / 13.0f);
}

void APOLLOAudioProcessor::loadRoomSimPreset (int roomType, int subPreset)
{
    struct PresetData {
        float inputGain, outputGain, roomSize, absorption, diffusion, predelay, decay, width, earlyLate, dry, wet;
    };

    // 8 room types x 5 sub-presets: Bright, Warm, Dark, Ambient, Tight
    //                                inputG  outG   size   abs    diff   predel decay  width  e/l    dry    wet
    static const PresetData presets[8][5] = {
        // 0: Theater
        {
            /* Bright  */ { 100.0f, 100.0f, 55.0f, 25.0f, 80.0f, 8.0f,  2.0f, 140.0f, 65.0f, 85.0f, 50.0f },
            /* Warm    */ { 100.0f, 100.0f, 50.0f, 45.0f, 65.0f, 12.0f, 2.5f, 110.0f, 50.0f, 85.0f, 50.0f },
            /* Dark    */ { 100.0f,  95.0f, 50.0f, 65.0f, 50.0f, 15.0f, 3.0f,  90.0f, 40.0f, 85.0f, 50.0f },
            /* Ambient */ { 100.0f,  95.0f, 70.0f, 20.0f, 85.0f, 30.0f, 4.5f, 160.0f, 30.0f, 60.0f, 75.0f },
            /* Tight   */ { 100.0f, 100.0f, 35.0f, 55.0f, 60.0f, 4.0f,  1.2f,  80.0f, 70.0f, 95.0f, 30.0f },
        },
        // 1: Church
        {
            /* Bright  */ {  95.0f, 100.0f, 75.0f, 20.0f, 80.0f, 20.0f, 3.5f, 150.0f, 60.0f, 80.0f, 60.0f },
            /* Warm    */ {  90.0f,  95.0f, 80.0f, 40.0f, 70.0f, 25.0f, 4.0f, 130.0f, 55.0f, 75.0f, 60.0f },
            /* Dark    */ {  90.0f,  90.0f, 80.0f, 60.0f, 55.0f, 30.0f, 5.0f, 100.0f, 40.0f, 75.0f, 60.0f },
            /* Ambient */ {  85.0f,  90.0f, 95.0f, 15.0f, 85.0f, 45.0f, 7.0f, 180.0f, 25.0f, 50.0f, 80.0f },
            /* Tight   */ {  95.0f, 100.0f, 55.0f, 50.0f, 65.0f, 10.0f, 2.0f,  90.0f, 70.0f, 90.0f, 35.0f },
        },
        // 2: Studio
        {
            /* Bright  */ { 100.0f, 100.0f, 20.0f, 40.0f, 70.0f, 3.0f,  1.0f, 110.0f, 60.0f, 95.0f, 35.0f },
            /* Warm    */ { 100.0f, 100.0f, 25.0f, 55.0f, 60.0f, 5.0f,  1.3f,  95.0f, 50.0f, 95.0f, 35.0f },
            /* Dark    */ { 100.0f,  95.0f, 22.0f, 70.0f, 45.0f, 6.0f,  1.6f,  75.0f, 40.0f, 95.0f, 35.0f },
            /* Ambient */ {  95.0f,  95.0f, 35.0f, 35.0f, 75.0f, 12.0f, 2.5f, 140.0f, 30.0f, 70.0f, 60.0f },
            /* Tight   */ { 100.0f, 100.0f, 12.0f, 65.0f, 55.0f, 2.0f,  0.6f,  70.0f, 75.0f, 100.0f, 20.0f },
        },
        // 3: Hall
        {
            /* Bright  */ { 100.0f, 100.0f, 60.0f, 25.0f, 80.0f, 15.0f, 2.5f, 140.0f, 60.0f, 80.0f, 55.0f },
            /* Warm    */ { 100.0f, 100.0f, 60.0f, 45.0f, 65.0f, 18.0f, 2.8f, 115.0f, 50.0f, 80.0f, 55.0f },
            /* Dark    */ { 100.0f,  95.0f, 60.0f, 60.0f, 50.0f, 22.0f, 3.5f,  90.0f, 38.0f, 80.0f, 55.0f },
            /* Ambient */ {  95.0f,  95.0f, 80.0f, 20.0f, 85.0f, 35.0f, 5.5f, 175.0f, 28.0f, 55.0f, 75.0f },
            /* Tight   */ { 100.0f, 100.0f, 40.0f, 50.0f, 60.0f, 6.0f,  1.5f,  85.0f, 72.0f, 92.0f, 30.0f },
        },
        // 4: Garage
        {
            /* Bright  */ { 100.0f, 100.0f, 30.0f, 15.0f, 45.0f, 4.0f,  1.5f, 100.0f, 65.0f, 90.0f, 40.0f },
            /* Warm    */ { 100.0f, 100.0f, 35.0f, 35.0f, 40.0f, 6.0f,  1.8f,  85.0f, 50.0f, 90.0f, 40.0f },
            /* Dark    */ { 100.0f,  95.0f, 35.0f, 55.0f, 30.0f, 8.0f,  2.2f,  70.0f, 38.0f, 90.0f, 40.0f },
            /* Ambient */ {  95.0f,  95.0f, 50.0f, 10.0f, 55.0f, 18.0f, 3.5f, 130.0f, 30.0f, 65.0f, 65.0f },
            /* Tight   */ { 100.0f, 100.0f, 20.0f, 40.0f, 35.0f, 2.0f,  0.8f,  65.0f, 75.0f, 95.0f, 25.0f },
        },
        // 5: Bathroom
        {
            /* Bright  */ { 100.0f, 100.0f, 10.0f, 10.0f, 35.0f, 1.0f,  0.8f,  80.0f, 70.0f, 90.0f, 45.0f },
            /* Warm    */ { 100.0f, 100.0f, 12.0f, 30.0f, 30.0f, 2.0f,  1.0f,  70.0f, 55.0f, 90.0f, 45.0f },
            /* Dark    */ { 100.0f,  95.0f, 12.0f, 50.0f, 25.0f, 3.0f,  1.3f,  55.0f, 40.0f, 90.0f, 45.0f },
            /* Ambient */ {  95.0f,  95.0f, 18.0f,  8.0f, 45.0f, 8.0f,  2.0f, 110.0f, 30.0f, 65.0f, 70.0f },
            /* Tight   */ { 100.0f, 100.0f,  6.0f, 35.0f, 30.0f, 0.5f,  0.4f,  50.0f, 80.0f, 100.0f, 20.0f },
        },
        // 6: Bedroom
        {
            /* Bright  */ { 100.0f, 100.0f, 22.0f, 45.0f, 60.0f, 4.0f,  1.0f, 100.0f, 60.0f, 92.0f, 35.0f },
            /* Warm    */ { 100.0f, 100.0f, 25.0f, 60.0f, 50.0f, 5.0f,  1.3f,  85.0f, 50.0f, 92.0f, 35.0f },
            /* Dark    */ { 100.0f,  95.0f, 25.0f, 75.0f, 40.0f, 7.0f,  1.6f,  65.0f, 38.0f, 92.0f, 35.0f },
            /* Ambient */ {  95.0f,  95.0f, 35.0f, 40.0f, 70.0f, 14.0f, 2.5f, 130.0f, 28.0f, 65.0f, 65.0f },
            /* Tight   */ { 100.0f, 100.0f, 14.0f, 65.0f, 50.0f, 2.0f,  0.6f,  60.0f, 72.0f, 100.0f, 20.0f },
        },
        // 7: Warehouse
        {
            /* Bright  */ { 100.0f, 100.0f, 85.0f, 15.0f, 60.0f, 12.0f, 3.0f, 160.0f, 55.0f, 85.0f, 45.0f },
            /* Warm    */ { 100.0f,  95.0f, 90.0f, 35.0f, 50.0f, 15.0f, 3.5f, 135.0f, 45.0f, 85.0f, 45.0f },
            /* Dark    */ { 100.0f,  90.0f, 90.0f, 55.0f, 40.0f, 20.0f, 4.5f, 105.0f, 35.0f, 85.0f, 45.0f },
            /* Ambient */ {  95.0f,  90.0f, 100.0f, 12.0f, 70.0f, 40.0f, 7.0f, 190.0f, 22.0f, 50.0f, 80.0f },
            /* Tight   */ { 100.0f, 100.0f, 60.0f, 40.0f, 45.0f, 5.0f,  1.8f,  90.0f, 68.0f, 92.0f, 30.0f },
        },
    };

    if (roomType < 0 || roomType >= 8 || subPreset < 0 || subPreset >= 5)
        return;

    const PresetData& p = presets[roomType][subPreset];

    apvts.getParameter ("rsInputGain")->setValueNotifyingHost (p.inputGain / 100.0f);
    apvts.getParameter ("rsOutputGain")->setValueNotifyingHost (p.outputGain / 100.0f);
    apvts.getParameter ("rsRoomSize")->setValueNotifyingHost ((p.roomSize - 1.0f) / 99.0f);
    apvts.getParameter ("rsAbsorption")->setValueNotifyingHost (p.absorption / 100.0f);
    apvts.getParameter ("rsDiffusion")->setValueNotifyingHost (p.diffusion / 100.0f);
    apvts.getParameter ("rsPredelay")->setValueNotifyingHost (p.predelay / 200.0f);
    apvts.getParameter ("rsDecay")->setValueNotifyingHost ((p.decay - 0.1f) / 29.9f);
    apvts.getParameter ("rsWidth")->setValueNotifyingHost (p.width / 200.0f);
    apvts.getParameter ("rsEarlyLate")->setValueNotifyingHost (p.earlyLate / 100.0f);
    apvts.getParameter ("rsDry")->setValueNotifyingHost (p.dry / 100.0f);
    apvts.getParameter ("rsWet")->setValueNotifyingHost (p.wet / 100.0f);
    apvts.getParameter ("rsRoomType")->setValueNotifyingHost (roomType / 7.0f);
}

// createPluginFilter removed — Apollo is hosted internally by ARK
