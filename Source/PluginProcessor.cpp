//PROCESSOR C++


/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
// PluginEditor.h is included AFTER PluginProcessor.h so that when
// PluginEditor.h tries to include PluginProcessor.h again, #pragma once
// blocks re-parsing. This is the standard JUCE pattern - the only way
// createEditor() can allocate a new ARKAudioProcessorEditor.
#include "PluginEditor.h"
#include <climits>

//==============================================================================
// Build the APVTS parameter layout
juce::AudioProcessorValueTreeState::ParameterLayout ARKAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // OSC A waveform: 0=Sine, 1=Saw, 2=Square, 3=Triangle
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID {"oscAWave", 1},
        "OSC A Waveform",
        0, 3, 0));

    // OSC B waveform: 0=Sine, 1=Saw, 2=Square, 3=Triangle
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID {"oscBWave", 1},
        "OSC B Waveform",
        0, 3, 1));

    // OSC A Power: true=on, false=off
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID {"oscAPower", 1},
        "OSC A Power",
        true)); // Default to ON

    // OSC B Power: true=on, false=off
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID {"oscBPower", 1},
        "OSC B Power",
        true)); // Default to ON

    // OSC A String Mode: 0=None, 1=Pluck, 2=Strum, 3=Pizz, 4=Arco
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID {"oscAStringMode", 1},
        "OSC A String Mode",
        juce::StringArray("None", "Pluck", "Strum", "Pizz", "Arco"),
        0));  // Default: None

    // OSC B String Mode: 0=None, 1=Pluck, 2=Strum, 3=Pizz, 4=Arco
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID {"oscBStringMode", 1},
        "OSC B String Mode",
        juce::StringArray("None", "Pluck", "Strum", "Pizz", "Arco"),
        0));  // Default: None

    // OSC A Choir Mode: 0=None, 1=OOH, 2=AAH, 3=Women, 4=Men
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID {"oscAChoirMode", 1},
        "OSC A Choir Mode",
        juce::StringArray("None", "OOH", "AAH", "Women", "Men"),
        0));  // Default: None

    // OSC B Choir Mode: 0=None, 1=OOH, 2=AAH, 3=Women, 4=Men
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID {"oscBChoirMode", 1},
        "OSC B Choir Mode",
        juce::StringArray("None", "OOH", "AAH", "Women", "Men"),
        0));  // Default: None

    // -------------------------------------------------------------------------
    // OSC A Knob Parameters
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscAOctave", 1},
        "OSC A Octave",
        juce::NormalisableRange<float>(-5.0f, 5.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscASemitone", 1},
        "OSC A Semitone",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 1.0f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscAFine", 1},
        "OSC A Fine",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscAUnison", 1},
        "OSC A Unison",
        juce::NormalisableRange<float>(1.0f, 16.0f, 1.0f),
        1.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscADetune", 1},
        "OSC A Detune",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscABlend", 1},
        "OSC A Blend",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscAWtPos", 1},
        "OSC A WT Pos",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscAPan", 1},
        "OSC A Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscALevel", 1},
        "OSC A Level",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        75.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscAPhase", 1},
        "OSC A Phase",
        juce::NormalisableRange<float>(0.0f, 360.0f, 1.0f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscASpread", 1},
        "OSC A Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscAChaos", 1},
        "OSC A Chaos",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // -------------------------------------------------------------------------
    // OSC B Knob Parameters
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBOctave", 1},
        "OSC B Octave",
        juce::NormalisableRange<float>(-5.0f, 5.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBSemitone", 1},
        "OSC B Semitone",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 1.0f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBFine", 1},
        "OSC B Fine",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBUnison", 1},
        "OSC B Unison",
        juce::NormalisableRange<float>(1.0f, 16.0f, 1.0f),
        1.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBDetune", 1},
        "OSC B Detune",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBBlend", 1},
        "OSC B Blend",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBWtPos", 1},
        "OSC B WT Pos",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBPan", 1},
        "OSC B Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBLevel", 1},
        "OSC B Level",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        75.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBPhase", 1},
        "OSC B Phase",
        juce::NormalisableRange<float>(0.0f, 360.0f, 1.0f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBSpread", 1},
        "OSC B Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"oscBChaos", 1},
        "OSC B Chaos",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // -------------------------------------------------------------------------
    // Filter Parameters
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterCutoff", 1},
        "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), // skewed for musical response
        20000.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterResonance", 1},
        "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterDrive", 1},
        "Filter Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID {"filterMode", 1},
        "Filter Mode",
        0, 3, 0)); // 0=LP 1=HP 2=BP 3=Notch

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID {"filterCharacter", 1},
        "Filter Character",
        0, 14, 0)); // 0=DEFAULT ... 14=AMBER

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterLfoAmount", 1},
        "Filter LFO Amount",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));

    // -------------------------------------------------------------------------
    // Sub Oscillator Parameters
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID {"subPower", 1},
        "Sub Power",
        true));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID {"subWave", 1},
        "Sub Waveform",
        0, 1, 0));  // 0=Sine, 1=Square

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"subOctave", 1},
        "Sub Octave",
        juce::NormalisableRange<float>(-5.0f, 5.0f, 1.0f),
        -1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"subLevel", 1},
        "Sub Level",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        75.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"subPan", 1},
        "Sub Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));

    // -------------------------------------------------------------------------
    // Noise Generator Parameters
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID {"noisePower", 1},
        "Noise Power",
        false));  // Default OFF

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID {"noiseType", 1},
        "Noise Type",
        0, 2, 0));  // 0=White, 1=Pink, 2=Brown

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"noiseLevel", 1},
        "Noise Level",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"noisePan", 1},
        "Noise Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID {"noiseGate", 1},
        "Noise Gate Mode",
        false));  // false=FREE (drone), true=KEY (triggered)

    // Noise Cutoff: simple tone control 20–20000 Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noiseCutoff", 1},
        "Noise Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
        20000.0f));

    // Noise LFO Amount: how much LFO 1 modulates noise level, 0–100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noiseLfoAmount", 1},
        "Noise LFO Amount",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // -------------------------------------------------------------------------
    // ENV 1 ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â Amplitude Envelope
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env1Attack", 1}, "ENV1 Attack",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env1Decay", 1}, "ENV1 Decay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 295.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env1Sustain", 1}, "ENV1 Sustain",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 70.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env1Release", 1}, "ENV1 Release",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 10.0f));

    // -------------------------------------------------------------------------
    // ENV 2 ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â Filter Envelope
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterEnvAttack", 1}, "Filter ENV Attack",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 10.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterEnvDecay", 1}, "Filter ENV Decay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 200.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterEnvSustain", 1}, "Filter ENV Sustain",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 70.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterEnvRelease", 1}, "Filter ENV Release",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 300.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"filterEnvAmount", 1}, "Filter ENV Amount",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

    // -------------------------------------------------------------------------
    // ENV 3 ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã…â€œ Assignable Modulation Envelope
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID {"env3Destination", 1}, "ENV3 Destination",
        0, 3, 0)); // 0=Filter Res, 1=Pitch, 2=Pan, 3=Drive

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env3Attack", 1}, "ENV3 Attack",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 10.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env3Decay", 1}, "ENV3 Decay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 200.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env3Sustain", 1}, "ENV3 Sustain",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 70.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env3Release", 1}, "ENV3 Release",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f), 300.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"env3Amount", 1}, "ENV3 Amount",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

    // =========================================================================
    // LFO Parameters (4 LFOs)
    // =========================================================================

    // LFO 1
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"lfo1Wave", 1},
        "LFO 1 Waveform",
        0, 3, 0)); // 0=Sine, 1=Saw, 2=Square, 3=Triangle

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1Rate", 1},
        "LFO 1 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1Rise", 1},
        "LFO 1 Rise",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1Delay", 1},
        "LFO 1 Delay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo1Smooth", 1},
        "LFO 1 Smooth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // LFO 2
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"lfo2Wave", 1},
        "LFO 2 Waveform",
        0, 3, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2Rate", 1},
        "LFO 2 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2Rise", 1},
        "LFO 2 Rise",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2Delay", 1},
        "LFO 2 Delay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo2Smooth", 1},
        "LFO 2 Smooth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // LFO 3
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"lfo3Wave", 1},
        "LFO 3 Waveform",
        0, 3, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo3Rate", 1},
        "LFO 3 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo3Rise", 1},
        "LFO 3 Rise",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo3Delay", 1},
        "LFO 3 Delay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo3Smooth", 1},
        "LFO 3 Smooth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // LFO 4
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"lfo4Wave", 1},
        "LFO 4 Waveform",
        0, 3, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo4Rate", 1},
        "LFO 4 Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo4Rise", 1},
        "LFO 4 Rise",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo4Delay", 1},
        "LFO 4 Delay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lfo4Smooth", 1},
        "LFO 4 Smooth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // LFO Trigger Modes: 0=TRIG (reset on note), 1=FREE (continuous), 2=OFF
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"lfo1TrigMode", 1},
        "LFO 1 Trigger Mode",
        0, 2, 0));  // Default: TRIG

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"lfo2TrigMode", 1},
        "LFO 2 Trigger Mode",
        0, 2, 0));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"lfo3TrigMode", 1},
        "LFO 3 Trigger Mode",
        0, 2, 0));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"lfo4TrigMode", 1},
        "LFO 4 Trigger Mode",
        0, 2, 0));


    // =========================================================================
    // CONTROLS Section Parameters (Voicing, Velocity, Note Tracking)
    // =========================================================================

    // Mono Mode: 0=POLY, 1=MONO
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"monoMode", 1},
        "Mono Mode",
        0, 1, 0));  // Default: POLY

    // Legato: 0=OFF, 1=ON
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"legato", 1},
        "Legato",
        0, 1, 0));  // Default: OFF

    // Portamento Time: 0-100 (maps to 0-5000ms internally)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"portaTime", 1},
        "Portamento Time",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // Portamento Curve: -100 to 100 (negative=log, 0=linear, positive=exp)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"portaCurve", 1},
        "Portamento Curve",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));

    // Portamento Always: 0=SCALED (proportional to interval), 1=ALWAYS
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"portaAlways", 1},
        "Portamento Always",
        0, 1, 1));  // Default: ALWAYS

    // Max Voices: 1-32
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"maxVoicesParam", 1},
        "Max Voices",
        1, 32, 16));  // Default: 16

    // Voice Steal Priority: 0=oldest, 1=quietest, 2=highest, 3=lowest
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"voiceStealMode", 1},
        "Voice Steal Mode",
        0, 3, 0));  // Default: oldest

    // Velocity Curve: -100 to 100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"veloCurve", 1},
        "Velocity Curve",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));

    // Note Tracking Curve: -100 to 100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noteTrackCurve", 1},
        "Note Tracking Curve",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));

    // =========================================================================
    // GLOBAL Page Parameters
    // =========================================================================

    // Pitch Bend Up: semitones 1-24, default 2
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"pitchBendUp", 1},
        "Pitch Bend Up",
        1, 24, 2));

    // Pitch Bend Down: semitones 1-24, default 2
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"pitchBendDown", 1},
        "Pitch Bend Down",
        1, 24, 2));

    // MIDI Channel: 0=ALL, 1-16
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"midiChannel", 1},
        "MIDI Channel",
        0, 16, 0));

    // MIDI Thru: 0=OFF, 1=ON
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"midiThru", 1},
        "MIDI Thru",
        0, 1, 0));

    // Mod Wheel Destination: 0=Filter Cutoff, 1=Filter Res, 2=Volume, 3=Pitch, 4=LFO Rate
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"modWheelDest", 1},
        "Mod Wheel Destination",
        0, 4, 0));

    // =========================================================================
    // ARP Parameters
    // =========================================================================

    // ARP On/Off: 0=OFF, 1=ON
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"arpOnOff", 1},
        "ARP On/Off",
        0, 1, 0));

    // ARP Mode (reserved)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"arpMode", 1},
        "ARP Mode",
        0, 1, 0));

    // ARP Rate: note division (1=whole, 2=half, 4=quarter, 8=eighth, 16=sixteenth)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"arpRate", 1},
        "ARP Rate",
        juce::NormalisableRange<float>(0.0f, 16.0f, 1.0f),
        4.0f));

    // ARP Gate: 0-100 (percentage of step length)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"arpGate", 1},
        "ARP Gate",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f));

    // ARP Swing: 0-100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"arpSwing", 1},
        "ARP Swing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // ARP Octave Range: 1-4
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"arpOctave", 1},
        "ARP Octave",
        1, 4, 1));

    // ARP Pattern: 0=Up, 1=Down, 2=UpDown, 3=Random, 4=Order
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"arpPattern", 1},
        "ARP Pattern",
        0, 4, 0));

    // ARP Latch: 0=OFF, 1=ON
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"arpLatch", 1},
        "ARP Latch",
        0, 1, 0));


    // =========================================================================
    // MOD Parameters
    // =========================================================================

    // Fattness: 0-10
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"modFattness", 1},
        "Mod Fattness",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f),
        5.0f));

    // HP Filter: 0-10
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"modHpFilter", 1},
        "Mod HP Filter",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f),
        1.0f));

    // FM: 0-100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"modFm", 1},
        "Mod FM",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // FM Filter: OSC B modulates filter cutoff, 0-100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"modFmFilter", 1},
        "Mod FM Filter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f));

    // Tighten: 0-10
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"modTighten", 1},
        "Mod Tighten",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f),
        2.0f));

    // Humanize: 0-10
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"modHuman", 1},
        "Mod Humanize",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f),
        0.0f));

    // =========================================================================
    // MASTER Parameters
    // =========================================================================

    // Master Volume: 0-100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterVolume", 1},
        "Master Volume",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        80.0f));

    // Master Tune: -100 to 100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterTune", 1},
        "Master Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));

    // Master Pan: -100 to 100
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterPan", 1},
        "Master Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
        0.0f));

    // Master Transpose: -24 to 24 semitones
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterTranspose", 1},
        "Master Transpose",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f),
        0.0f));

    // =========================================================================
    // MODULATION MATRIX — 16 slots
    // Sources:  0=None 1=LFO1 2=LFO2 3=LFO3 4=LFO4 5=ENV1 6=ENV2 7=ENV3
    //           8=Velocity 9=Note 10=ModWheel 11=PitchBend
    // Destinations: 0=None 1=OscAPitch 2=OscALevel 3=OscAPan 4=OscAWtPos 5=OscADetune
    //               6=OscBPitch 7=OscBLevel 8=OscBPan 9=OscBWtPos 10=OscBDetune
    //               11=FilterCutoff 12=FilterRes 13=FilterDrive
    //               14=Env1Atk 15=Env1Dec 16=Env1Sus 17=Env1Rel
    //               18=Lfo1Rate 19=Lfo2Rate 20=Lfo3Rate 21=Lfo4Rate
    //               22=MasterVol 23=MasterPan
    // =========================================================================
    for (int i = 1; i <= 16; ++i)
    {
        juce::String idx = juce::String(i);

        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"matrixSource" + idx, 1},
            "Matrix Source " + idx,
            0, 11, 0));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"matrixDest" + idx, 1},
            "Matrix Dest " + idx,
            0, 23, 0));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"matrixAmount" + idx, 1},
            "Matrix Amount " + idx,
            juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
            0.0f));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"matrixEnabled" + idx, 1},
            "Matrix Enabled " + idx,
            false));
    }

    return layout;
}

//==============================================================================
ARKAudioProcessor::ARKAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif // ! JucePlugin_IsMidiEffect
                       ),
#else
     :
#endif
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    choirFormatManager.registerBasicFormats();
    loadChoirSamples();
    refreshOscPresetList();
}

ARKAudioProcessor::~ARKAudioProcessor()
{
}

//==============================================================================
// Loads the four choir sample WAVs from <project>/Samples/Choir/
// Falls back gracefully — if a file is missing, that mode produces silence.
void ARKAudioProcessor::loadChoirSamples()
{
    juce::File samplesDir;

    // Write a debug log to Desktop so we can see what paths the plugin resolves
    juce::File debugLog = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                              .getChildFile("ARK_choir_debug.txt");
    juce::String debugText;

    juce::File execFile = juce::File::getSpecialLocation(
                              juce::File::currentExecutableFile);
    debugText += "execFile: " + execFile.getFullPathName() + "\n";

    juce::File appFile = juce::File::getSpecialLocation(
                             juce::File::currentApplicationFile);
    debugText += "appFile: " + appFile.getFullPathName() + "\n";

    // Walk up from <bundle>/Contents/MacOS/ARK to <bundle>
    juce::File bundleDir = execFile.getParentDirectory()   // MacOS/
                                   .getParentDirectory()   // Contents/
                                   .getParentDirectory();  // <bundle>.component or .vst3
    debugText += "bundleDir: " + bundleDir.getFullPathName() + "\n";

    juce::File bundleResources = bundleDir.getChildFile("Contents/Resources/Samples/Choir");
    debugText += "bundleResources: " + bundleResources.getFullPathName() + "\n";
    debugText += "bundleResources isDir: " + juce::String(bundleResources.isDirectory() ? "YES" : "NO") + "\n";

    if (bundleResources.isDirectory())
    {
        samplesDir = bundleResources;
    }
    else
    {
        // Fallback: walk up from the binary to find <project>/Samples/Choir
        juce::File searchDir = execFile.getParentDirectory();
        for (int i = 0; i < 8; ++i)
        {
            juce::File candidate = searchDir.getChildFile("Samples/Choir");
            debugText += "fallback[" + juce::String(i) + "]: " + candidate.getFullPathName()
                         + " isDir=" + juce::String(candidate.isDirectory() ? "YES" : "NO") + "\n";
            if (candidate.isDirectory())
            {
                samplesDir = candidate;
                break;
            }
            searchDir = searchDir.getParentDirectory();
        }
    }

    debugText += "FINAL samplesDir: " + samplesDir.getFullPathName() + "\n";
    debugText += "FINAL isDir: " + juce::String(samplesDir.isDirectory() ? "YES" : "NO") + "\n";
    debugLog.replaceWithText(debugText);

    if (!samplesDir.isDirectory())
        return;  // Samples folder not found — all modes will be silent

    const char* fileNames[numChoirSamples] = {
        "choir_ooh.wav",
        "choir_aah.wav",
        "choir_women.wav",
        "choir_men.wav"
    };

    for (int i = 0; i < numChoirSamples; ++i)
    {
        juce::File f = samplesDir.getChildFile(fileNames[i]);
        debugText += "File[" + juce::String(i) + "]: " + f.getFullPathName()
                     + " exists=" + juce::String(f.existsAsFile() ? "YES" : "NO") + "\n";
        if (!f.existsAsFile()) continue;

        std::unique_ptr<juce::AudioFormatReader> reader(
            choirFormatManager.createReaderFor(f));
        debugText += "  reader=" + juce::String(reader != nullptr ? "OK" : "NULL") + "\n";
        if (reader == nullptr) continue;

        int numCh      = (int)reader->numChannels;
        int numSamples = (int)reader->lengthInSamples;
        debugText += "  channels=" + juce::String(numCh) + " samples=" + juce::String(numSamples)
                     + " sampleRate=" + juce::String(reader->sampleRate) + "\n";

        // Clamp to stereo
        int loadCh = juce::jmin(numCh, 2);
        choirSampleBuffers[i].setSize(loadCh, numSamples);
        reader->read(&choirSampleBuffers[i], 0, numSamples, 0, true, loadCh > 1);

        choirSamplesLoaded[i] = true;
        debugText += "  LOADED OK, buffer size=" + juce::String(choirSampleBuffers[i].getNumSamples()) + "\n";
    }

    // Count how many loaded
    int loadedCount = 0;
    for (int i = 0; i < numChoirSamples; ++i)
        if (choirSamplesLoaded[i]) loadedCount++;
    debugText += "Total loaded: " + juce::String(loadedCount) + "/" + juce::String(numChoirSamples) + "\n";
    debugLog.replaceWithText(debugText);
}

//==============================================================================
// Creates all 4 wavetables: Sine, Saw, Square, Triangle
void ARKAudioProcessor::createWavetables()
{
    for (int w = 0; w < numWaveforms; ++w)
    {
        wavetables[w].setSize (1, wavetableSize);
        auto* samples = wavetables[w].getWritePointer (0);

        for (int i = 0; i < wavetableSize; ++i)
        {
            float t = (float)i / (float)wavetableSize;  // 0.0 -> 1.0

            switch (w)
            {
                case 0: // Sine
                    samples[i] = std::sin (juce::MathConstants<float>::twoPi * t);
                    break;

                case 1: // Saw  (ramps +1 -> -1)
                    samples[i] = 1.0f - 2.0f * t;
                    break;

                case 2: // Square
                    samples[i] = (t < 0.5f) ? 1.0f : -1.0f;
                    break;

                case 3: // Triangle
                    samples[i] = (t < 0.5f) ? (4.0f * t - 1.0f)
                                             : (3.0f - 4.0f * t);
                    break;

                default:
                    samples[i] = 0.0f;
                    break;
            }
        }
    }
}

// Get next sample for a specific voice using the chosen wavetable
float ARKAudioProcessor::getNextSampleForVoice (Voice& voice, int wavetableIndex,
                                                 float octave, float semitone, float fine,
                                                 float level, float phase, float wtPos) noexcept
{
    // Calculate frequency using portamento-aware currentFrequency
    float pitchShift = (octave * 12.0f) + semitone + (fine / 100.0f);
    float frequency  = voice.currentFrequency * std::pow (2.0f, pitchShift / 12.0f);
    float tableDelta = (float)wavetableSize * frequency / (float)currentSampleRate;

    // Apply phase offset
    float phaseOffset = (phase / 360.0f) * wavetableSize;
    float readIndex = voice.currentIndex + phaseOffset;
    
    // Wrap read index
    while (readIndex >= wavetableSize)
        readIndex -= wavetableSize;
    while (readIndex < 0.0f)
        readIndex += wavetableSize;
    
    // Wavetable morphing based on wtPos (0-100)
    // 0-25: Sine to Saw, 25-50: Saw to Square, 50-75: Square to Triangle, 75-100: Triangle to Sine
    float morphPosition = wtPos / 100.0f * 4.0f; // Convert to 0-4 range
    int table1Index = (int)morphPosition;
    int table2Index = (table1Index + 1) % numWaveforms;
    float morphBlend = morphPosition - (float)table1Index; // 0-1 blend factor
    
    // Override table indices if wtPos is 0 (use selected waveform only)
    if (wtPos < 0.1f)
    {
        table1Index = wavetableIndex;
        table2Index = wavetableIndex;
        morphBlend = 0.0f;
    }
    
    // Clamp table indices
    table1Index = juce::jlimit(0, numWaveforms - 1, table1Index);
    table2Index = juce::jlimit(0, numWaveforms - 1, table2Index);
    
    // Read from first wavetable
    auto* table1 = wavetables[table1Index].getReadPointer(0);
    int index0 = (int)readIndex;
    int index1 = (index0 + 1) % wavetableSize;
    float frac = readIndex - index0;
    float value1 = table1[index0] + frac * (table1[index1] - table1[index0]);
    
    // Read from second wavetable
    auto* table2 = wavetables[table2Index].getReadPointer(0);
    float value2 = table2[index0] + frac * (table2[index1] - table2[index0]);
    
    // Blend between the two wavetables
    float value = value1 + morphBlend * (value2 - value1);

    // Increment voice position for next sample
    voice.currentIndex += tableDelta;
    if (voice.currentIndex >= wavetableSize)
        voice.currentIndex -= wavetableSize;

    // Apply level
    float levelMultiplier = level / 100.0f;
    return value * voice.level * levelMultiplier;
}

//==============================================================================
// String Synthesis using Harmonic Decay with Physics-Based Damping
// Uses per-oscillator StringState so OSC A and OSC B are fully independent.
//==============================================================================
float ARKAudioProcessor::getNextSampleForString(Voice::StringState& ss,
                                                float voiceLevel,
                                                float frequency,
                                                int stringMode,
                                                int numHarmonics) noexcept
{
    // -------------------------------------------------------------------------
    // Initialize on new note (mode changed or energy depleted)
    // -------------------------------------------------------------------------
    if (ss.stringMode != (unsigned int)stringMode || ss.excitationEnergy < 1e-6f)
    {
        ss.stringMode = (unsigned int)stringMode;

        for (int h = 0; h < numHarmonics; ++h)
        {
            ss.harmonicAmplitude[h] = 1.0f / (float)(h + 1);
            ss.harmonicPhase[h] = 0.0f;
        }

        ss.excitationEnergy = 1.0f;

        // Per-mode damping and strike position
        switch (stringMode)
        {
            case 1:  ss.stringDamping = 0.999969f; ss.strikePosition = 0.13f; break; // Pluck ~5s
            case 2:  ss.stringDamping = 0.999969f; ss.strikePosition = 0.15f; break; // Strum ~5s
            case 3:  ss.stringDamping = 0.999895f; ss.strikePosition = 0.20f; break; // Pizz ~1.5s
            case 4:  ss.stringDamping = 0.999999f; ss.strikePosition = 0.10f; break; // Arco
            default: ss.stringDamping = 0.999969f; ss.strikePosition = 0.15f; break;
        }

        // Precompute per-harmonic decay rates (avoids std::pow every sample)
        for (int h = 0; h < numHarmonics; ++h)
        {
            float qFactor = 1.0f + (float)h * 0.08f;
            ss.harmonicDecayRate[h] = std::pow(ss.stringDamping, qFactor);
        }
    }

    if (frequency < 20.0f)   frequency = 20.0f;
    if (frequency > 18000.0f) frequency = 18000.0f;

    const float twoPi = juce::MathConstants<float>::twoPi;
    const float pi    = juce::MathConstants<float>::pi;
    const float sr    = (float)getSampleRate();

    // -------------------------------------------------------------------------
    // Accumulate harmonics
    // -------------------------------------------------------------------------
    float output = 0.0f;

    for (int h = 0; h < numHarmonics; ++h)
    {
        float harmonicFreq = frequency * (float)(h + 1);

        if (harmonicFreq > sr * 0.5f)
            break;

        float phaseInc = (harmonicFreq / sr) * twoPi;
        ss.harmonicPhase[h] += phaseInc;
        if (ss.harmonicPhase[h] >= twoPi)
            ss.harmonicPhase[h] -= twoPi;

        float strikeFactor = std::abs(std::sin(pi * (float)(h + 1) * ss.strikePosition));
        float sample = std::sin(ss.harmonicPhase[h]);
        output += ss.harmonicAmplitude[h] * strikeFactor * sample;
    }

    output *= 0.25f;
    output *= ss.excitationEnergy;

    // -------------------------------------------------------------------------
    // Decay (skip for Arco — bow sustains indefinitely)
    // -------------------------------------------------------------------------
    if (stringMode != 4)
    {
        ss.excitationEnergy *= ss.stringDamping;

        for (int h = 0; h < numHarmonics; ++h)
            ss.harmonicAmplitude[h] *= ss.harmonicDecayRate[h];
    }

    output *= voiceLevel;
    return output;
}

//==============================================================================
// Choir Synthesis — LF Glottal Source + Formant Resonator Bank
//==============================================================================
// Sample-based choir engine.
// Loads the correct buffer for the requested mode, calculates a pitch ratio
// from the voice frequency vs the sample root (D3 = 174.614 Hz), and
// advances a fractional read position with linear interpolation.
// Loops the sustain region (20%–80% of the buffer) for held notes.
//==============================================================================
float ARKAudioProcessor::getNextSampleForChoir(Voice::ChoirSamplerState& cs,
                                                float voiceLevel,
                                                float frequency,
                                                int choirMode) noexcept
{
    if (choirMode <= 0 || choirMode > 4) return 0.0f;

    const int bufIndex = choirMode - 1;  // 0=OOH, 1=AAH, 2=Women, 3=Men

    if (!choirSamplesLoaded[bufIndex]) return 0.0f;

    const juce::AudioBuffer<float>& buf = choirSampleBuffers[bufIndex];
    const int   totalSamples = buf.getNumSamples();
    const int   numCh        = buf.getNumChannels();
    if (totalSamples < 2) return 0.0f;

    // Reset read position when mode changes or voice restarts
    if (cs.choirMode != choirMode || !cs.isPlaying)
    {
        cs.choirMode  = choirMode;
        cs.readPos    = 0.0;
        cs.isPlaying  = true;
    }

    // Pitch ratio: how fast to advance through the buffer
    // root is D3 = 174.614 Hz; also account for sample rate difference
    const double rootHz       = choirSampleRoots[bufIndex];  // 174.614 Hz
    const double bufferSR     = (bufIndex == 0 || bufIndex == 1) ? 96000.0
                              : (bufIndex == 2)                   ? 44100.0
                                                                  : 48000.0;
    const double pitchRatio   = (frequency / rootHz) * (bufferSR / currentSampleRate);

    // Sustain loop region (use middle 60% of the buffer)
    const int loopStart = (int)(totalSamples * 0.20);
    const int loopEnd   = (int)(totalSamples * 0.80);

    // Advance position, looping in sustain zone once we reach it
    cs.readPos += pitchRatio;

    if (cs.readPos >= loopEnd)
        cs.readPos = loopStart + std::fmod(cs.readPos - loopStart,
                                           (double)(loopEnd - loopStart));

    if (cs.readPos >= totalSamples)
        cs.readPos = loopStart;

    // Linear interpolation
    int   i0   = (int)cs.readPos;
    int   i1   = i0 + 1;
    float frac = (float)(cs.readPos - i0);

    i0 = juce::jlimit(0, totalSamples - 1, i0);
    i1 = juce::jlimit(0, totalSamples - 1, i1);

    // Mix channels to mono (most buffers are stereo)
    float s0 = 0.0f, s1 = 0.0f;
    for (int c = 0; c < numCh; ++c)
    {
        s0 += buf.getSample(c, i0);
        s1 += buf.getSample(c, i1);
    }
    if (numCh > 1) { s0 *= 0.5f; s1 *= 0.5f; }

    float output = s0 + frac * (s1 - s0);

    // Per-mode gain compensation — Women sample is very hot, Men is quiet
    static const float modeGain[4] = { 1.0f,   // OOH
                                       1.0f,   // AAH
                                       0.35f,  // Women  (turn down)
                                       4.5f }; // Men    (turn up)
    output *= modeGain[bufIndex];

    output *= voiceLevel;

    return output;
}

// ============================================================================
// Voicing / Controls Helper Functions
// ============================================================================

void ARKAudioProcessor::pushHeldNote(int midiNote)
{
    // Remove if already present (to move to top of stack)
    removeHeldNote(midiNote);
    if (heldNoteCount < 128)
        heldNotes[heldNoteCount++] = midiNote;
}

void ARKAudioProcessor::removeHeldNote(int midiNote)
{
    for (int i = 0; i < heldNoteCount; ++i)
    {
        if (heldNotes[i] == midiNote)
        {
            for (int j = i; j < heldNoteCount - 1; ++j)
                heldNotes[j] = heldNotes[j + 1];
            --heldNoteCount;
            return;
        }
    }
}

int ARKAudioProcessor::topHeldNote() const
{
    return (heldNoteCount > 0) ? heldNotes[heldNoteCount - 1] : -1;
}

Voice* ARKAudioProcessor::findOldestVoice()
{
    Voice* oldest = nullptr;
    unsigned long long minTime = ULLONG_MAX;
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].isActive && voices[i].voiceStartTime < minTime)
        {
            minTime = voices[i].voiceStartTime;
            oldest = &voices[i];
        }
    }
    return oldest;
}

Voice* ARKAudioProcessor::findQuietestVoice()
{
    Voice* quietest = nullptr;
    float minLevel = 999.0f;
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].isActive && voices[i].ampEnvLevel < minLevel)
        {
            minLevel = voices[i].ampEnvLevel;
            quietest = &voices[i];
        }
    }
    return quietest;
}

Voice* ARKAudioProcessor::allocateVoice(int maxAllowed)
{
    // Count non-releasing active voices to enforce the user's voice cap
    int activePlayingCount = 0;
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].isActive && voices[i].ampStage != Voice::EnvStage::Release)
            ++activePlayingCount;
    }

    // 1. Under the cap — prefer a releasing voice first (least disruptive),
    //    then fall back to any idle slot.
    if (activePlayingCount < maxAllowed)
    {
        Voice* oldestReleasing = nullptr;
        unsigned long long minTime = ULLONG_MAX;
        for (int i = 0; i < maxVoices; ++i)
        {
            if (voices[i].ampStage == Voice::EnvStage::Release
                && voices[i].voiceStartTime < minTime)
            {
                minTime = voices[i].voiceStartTime;
                oldestReleasing = &voices[i];
            }
        }
        if (oldestReleasing != nullptr)
            return oldestReleasing;

        for (int i = 0; i < maxVoices; ++i)
        {
            if (!voices[i].isActive)
                return &voices[i];
        }

        return nullptr; // all slots active but under cap — shouldn't happen
    }

    // 2. At or above the cap — must steal an active playing voice.
    //    Prefer stealing the oldest releasing voice first.
    {
        Voice* oldestReleasing = nullptr;
        unsigned long long minTime = ULLONG_MAX;
        for (int i = 0; i < maxVoices; ++i)
        {
            if (voices[i].ampStage == Voice::EnvStage::Release
                && voices[i].voiceStartTime < minTime)
            {
                minTime = voices[i].voiceStartTime;
                oldestReleasing = &voices[i];
            }
        }
        if (oldestReleasing != nullptr)
            return oldestReleasing;
    }

    // No releasing voices — steal a playing voice based on priority mode
    int stealMode = (int)apvts.getRawParameterValue("voiceStealMode")->load();
    switch (stealMode)
    {
        case 1: return findQuietestVoice();
        case 2: // highest note
        {
            Voice* highest = nullptr;
            int maxNote = -1;
            for (int i = 0; i < maxVoices; ++i)
                if (voices[i].isActive && voices[i].midiNote > maxNote)
                    { maxNote = voices[i].midiNote; highest = &voices[i]; }
            return highest;
        }
        case 3: // lowest note
        {
            Voice* lowest = nullptr;
            int minNote = 128;
            for (int i = 0; i < maxVoices; ++i)
                if (voices[i].isActive && voices[i].midiNote < minNote)
                    { minNote = voices[i].midiNote; lowest = &voices[i]; }
            return lowest;
        }
        default: return findOldestVoice(); // case 0 and fallback
    }
}

float ARKAudioProcessor::applyPortaCurve(float progress, float curveParam) const
{
    if (std::abs(curveParam) < 0.1f)
        return progress; // linear

    if (curveParam > 0.0f)
    {
        float exp = 1.0f + (curveParam / 100.0f) * 3.0f;
        return std::pow(progress, exp); // exponential (slow start, fast end)
    }
    else
    {
        float exp = 1.0f / (1.0f + (-curveParam / 100.0f) * 3.0f);
        return std::pow(progress, exp); // logarithmic (fast start, slow end)
    }
}

float ARKAudioProcessor::shapeVelocity(float rawVelocity, float curveParam) const
{
    if (std::abs(curveParam) < 0.1f)
        return rawVelocity; // linear

    if (curveParam > 0.0f)
    {
        float exp = 1.0f + (curveParam / 100.0f) * 3.0f;
        return std::pow(rawVelocity, exp);
    }
    else
    {
        float exp = 1.0f / (1.0f + (-curveParam / 100.0f) * 3.0f);
        return std::pow(rawVelocity, exp);
    }
}

float ARKAudioProcessor::calculateNoteTracking(int midiNote, float curveParam) const
{
    // Map MIDI note 0-127 to -1..+1, centered at middle C (note 60)
    float t = (float)(midiNote - 60) / 60.0f;
    t = juce::jlimit(-1.0f, 1.0f, t);

    if (std::abs(curveParam) < 0.1f)
        return t; // linear

    float shaped = std::pow(std::abs(t), 1.0f + std::abs(curveParam) / 50.0f);
    return t < 0.0f ? -shaped : shaped;
}


// ============================================================================

// ============================================================================
// ARP Helper Functions
// ============================================================================

void ARKAudioProcessor::arpPushNote(int midiNote)
{
    // Remove if already present (to maintain arrival order)
    arpRemoveNote(midiNote);
    if (arpHeldNoteCount < 128)
    {
        arpHeldNotes[arpHeldNoteCount] = midiNote;
        arpHeldNoteCount++;
    }
}

void ARKAudioProcessor::arpRemoveNote(int midiNote)
{
    for (int i = 0; i < arpHeldNoteCount; ++i)
    {
        if (arpHeldNotes[i] == midiNote)
        {
            for (int j = i; j < arpHeldNoteCount - 1; ++j)
                arpHeldNotes[j] = arpHeldNotes[j + 1];
            arpHeldNoteCount--;
            return;
        }
    }
}

void ARKAudioProcessor::arpBuildSequence(int pattern, int octaveRange)
{
    arpSequenceLength = 0;
    if (arpHeldNoteCount == 0) return;

    // Copy held notes and sort for Up/Down/UpDown patterns
    int sorted[128];
    for (int i = 0; i < arpHeldNoteCount; ++i)
        sorted[i] = arpHeldNotes[i];
    int count = arpHeldNoteCount;

    // Simple insertion sort (small array)
    if (pattern != 4) // Don't sort for Order mode
    {
        for (int i = 1; i < count; ++i)
        {
            int key = sorted[i];
            int j = i - 1;
            while (j >= 0 && sorted[j] > key)
            {
                sorted[j + 1] = sorted[j];
                j--;
            }
            sorted[j + 1] = key;
        }
    }

    switch (pattern)
    {
        case 0: // Up
            for (int oct = 0; oct < octaveRange; ++oct)
                for (int i = 0; i < count; ++i)
                    if (arpSequenceLength < 512)
                        arpSequence[arpSequenceLength++] = sorted[i] + oct * 12;
            break;

        case 1: // Down
            for (int oct = octaveRange - 1; oct >= 0; --oct)
                for (int i = count - 1; i >= 0; --i)
                    if (arpSequenceLength < 512)
                        arpSequence[arpSequenceLength++] = sorted[i] + oct * 12;
            break;

        case 2: // UpDown
        {
            // Up phase
            for (int oct = 0; oct < octaveRange; ++oct)
                for (int i = 0; i < count; ++i)
                    if (arpSequenceLength < 512)
                        arpSequence[arpSequenceLength++] = sorted[i] + oct * 12;
            // Down phase - skip duplicate at peak and trough
            for (int oct = octaveRange - 1; oct >= 0; --oct)
            {
                int start = (oct == octaveRange - 1) ? count - 2 : count - 1;
                int end   = (oct == 0) ? 1 : 0;
                for (int i = start; i >= end; --i)
                    if (arpSequenceLength < 512)
                        arpSequence[arpSequenceLength++] = sorted[i] + oct * 12;
            }
            break;
        }

        case 3: // Random - build same as Up but will be picked randomly at step time
            for (int oct = 0; oct < octaveRange; ++oct)
                for (int i = 0; i < count; ++i)
                    if (arpSequenceLength < 512)
                        arpSequence[arpSequenceLength++] = sorted[i] + oct * 12;
            break;

        case 4: // Order (arrival order)
            for (int oct = 0; oct < octaveRange; ++oct)
                for (int i = 0; i < count; ++i)
                    if (arpSequenceLength < 512)
                        arpSequence[arpSequenceLength++] = arpHeldNotes[i] + oct * 12;
            break;
    }
}

void ARKAudioProcessor::arpNoteOn(int midiNote, float velocity)
{
    // Use allocateVoice so releasing voices are preferred for stealing
    int maxVoicesCap = (int)apvts.getRawParameterValue("maxVoicesParam")->load();
    Voice* voice = allocateVoice(maxVoicesCap);
    if (voice == nullptr)
        return;

    float freq = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);

    voice->isActive = true;
    voice->midiNote = midiNote;
    voice->level = velocity;
    voice->currentIndex = 0.0f;
    voice->subIndex = 0.0f;
    voice->targetFrequency = freq;
    voice->currentFrequency = freq;
    voice->portaProgress = 1.0f;
    voice->shapedVelocity = velocity;
    voice->noteTrackValue = 0.0f;
    voice->voiceStartTime = voiceCounter++;

    // Trigger envelopes
    voice->ampStage = Voice::EnvStage::Attack;
    voice->ampEnvLevel = 0.0f;
    voice->filterStage = Voice::EnvStage::Attack;
    voice->filterEnvLevel = 0.0f;
    voice->env3Stage = Voice::EnvStage::Attack;
    voice->env3Level = 0.0f;

    // Reset string synthesis state for new note (both oscillators)
    voice->stringA.excitationEnergy = 0.0f;
    voice->stringA.stringMode = 0;
    voice->stringB.excitationEnergy = 0.0f;
    voice->stringB.stringMode = 0;

    // Reset choir synthesis state for new note (both oscillators)
        voice->choirA.reset();
        voice->choirB.reset();
    
    // Reset LFOs (check trigger mode)
    for (int l = 0; l < 4; ++l)
    {
        const char* trigIds[] = {"lfo1TrigMode","lfo2TrigMode","lfo3TrigMode","lfo4TrigMode"};
        int trigMode = (int)apvts.getRawParameterValue(trigIds[l])->load();
        if (trigMode == 0) // TRIG mode - reset on note
        {
            voice->lfoPhase[l] = 0.0f;
            voice->lfoValue[l] = 0.0f;
            voice->lfoSmoothed[l] = 0.0f;
            voice->lfoActive[l] = false;
            voice->lfoRiseLevel[l] = 0.0f;
            const char* delayIds[] = {"lfo1Delay","lfo2Delay","lfo3Delay","lfo4Delay"};
            float delayMs = apvts.getRawParameterValue(delayIds[l])->load();
            voice->lfoDelayTime[l] = (delayMs / 1000.0f) * (float)currentSampleRate;
        }
    }

    arpCurrentNote = midiNote;
    arpNoteIsOn = true;
}

void ARKAudioProcessor::arpNoteOff(int midiNote)
{
    // Release ALL voices playing this note, not just the first match.
    // With longer release times, multiple voices can share the same midiNote
    // (the previous one still releasing + the current one sustaining).
    // If we only release the first match, the newer voice can get stuck.
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].isActive && voices[i].midiNote == midiNote
            && voices[i].ampStage != Voice::EnvStage::Release)
        {
            voices[i].ampStage = Voice::EnvStage::Release;
            voices[i].filterStage = Voice::EnvStage::Release;
            voices[i].env3Stage = Voice::EnvStage::Release;
        }
    }
    if (arpCurrentNote == midiNote)
    {
        arpNoteIsOn = false;
        arpCurrentNote = -1;
    }
}

void ARKAudioProcessor::arpReset()
{
    // Turn off current arp note if playing
    if (arpNoteIsOn && arpCurrentNote >= 0)
        arpNoteOff(arpCurrentNote);

    arpHeldNoteCount = 0;
    arpSequenceLength = 0;
    arpCurrentStep = 0;
    arpCurrentNote = -1;
    arpSamplesUntilNext = 0;
    arpSamplesUntilGateOff = 0;
    arpNoteIsOn = false;
    arpGoingUp = true;
    arpLastVelocity = 0.8f;
}

// LFO Helper Functions
// ============================================================================

// Calculate LFO waveform output from phase (0-1)
float ARKAudioProcessor::calculateLFOWaveform(float phase, int waveType) noexcept
{
    switch (waveType)
    {
        case 0: // Sine
            return std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
        
        case 1: // Saw (downward ramp)
            return 1.0f - 2.0f * phase;
        
        case 2: // Square
            return (phase < 0.5f) ? 1.0f : -1.0f;
        
        case 3: // Triangle
            if (phase < 0.5f)
                return 4.0f * phase - 1.0f;  // Rising edge
            else
                return 3.0f - 4.0f * phase;  // Falling edge
        
        default:
            return 0.0f;
    }
}

// Update LFO state for a voice
void ARKAudioProcessor::updateLFO(Voice& voice, int lfoIndex, float rate, float rise, float delay, float smooth, int waveType, int trigMode) noexcept
{
    // OFF mode: output zero and return
    if (trigMode == 2)
    {
        voice.lfoValue[lfoIndex] = 0.0f;
        voice.lfoSmoothed[lfoIndex] = 0.0f;
        return;
    }

    // Handle delay phase
    if (voice.lfoDelayTime[lfoIndex] > 0.0f)
    {
        voice.lfoDelayTime[lfoIndex] -= 1.0f;
        voice.lfoValue[lfoIndex] = 0.0f;
        voice.lfoSmoothed[lfoIndex] = 0.0f;
        return;
    }
    
    // LFO is now active
    voice.lfoActive[lfoIndex] = true;
    
    // Update phase
    float phaseIncrement = rate / (float)currentSampleRate;
    voice.lfoPhase[lfoIndex] += phaseIncrement;
    
    // Wrap phase
    while (voice.lfoPhase[lfoIndex] >= 1.0f)
        voice.lfoPhase[lfoIndex] -= 1.0f;
    
    // Calculate raw waveform
    float rawValue = calculateLFOWaveform(voice.lfoPhase[lfoIndex], waveType);
    
    // Apply rise (fade-in)
    if (rise > 0.1f)
    {
        float riseTime = (rise / 100.0f) * 2000.0f;  // 0-100 maps to 0-2000ms
        float riseSamples = riseTime * (float)currentSampleRate / 1000.0f;
        
        if (voice.lfoRiseLevel[lfoIndex] < 1.0f)
        {
            voice.lfoRiseLevel[lfoIndex] += 1.0f / riseSamples;
            if (voice.lfoRiseLevel[lfoIndex] > 1.0f)
                voice.lfoRiseLevel[lfoIndex] = 1.0f;
        }
        
        rawValue *= voice.lfoRiseLevel[lfoIndex];
    }
    else
    {
        voice.lfoRiseLevel[lfoIndex] = 1.0f;
    }
    
    // Store raw value
    voice.lfoValue[lfoIndex] = rawValue;
    
    // Apply smoothing (low-pass filtering)
    if (smooth > 0.1f)
    {
        float smoothFactor = 1.0f - (smooth / 100.0f * 0.99f);  // 0-99% smoothing
        voice.lfoSmoothed[lfoIndex] = voice.lfoSmoothed[lfoIndex] * smoothFactor +
                                      rawValue * (1.0f - smoothFactor);
    }
    else
    {
        voice.lfoSmoothed[lfoIndex] = rawValue;
    }
}

//==============================================================================
const juce::String ARKAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ARKAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ARKAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ARKAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ARKAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ARKAudioProcessor::getNumPrograms()
{
    return 1;
}

int ARKAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ARKAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ARKAudioProcessor::getProgramName (int index)
{
    return {};
}

void ARKAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ARKAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Build all 4 wavetables
    createWavetables();
    
    // Initialize all voices as inactive
    for (int i = 0; i < maxVoices; ++i)
    {
        voices[i].isActive = false;
        voices[i].midiNote = -1;
        voices[i].currentIndex = 0.0f;
        voices[i].subIndex = 0.0f;
        voices[i].level = 0.0f;
        voices[i].targetFrequency = 440.0f;
        voices[i].currentFrequency = 440.0f;
        voices[i].portaStartFrequency = 440.0f;
        voices[i].portaProgress = 1.0f;
        voices[i].lastMidiNote = -1;
        voices[i].shapedVelocity = 0.0f;
        voices[i].noteTrackValue = 0.0f;
        voices[i].voiceStartTime = 0;
    }
    
    // Reset keyboard state
    keyboardState.reset();

    // Reset voicing state
    heldNoteCount = 0;
    voiceCounter = 0;

    // Reset ARP state
    arpReset();

    // Reset FM filter accumulator
    oscBFmAccum = 0.0f;

    // Prepare the Moog ladder filters
    filterLeft.prepare  (sampleRate);
    filterRight.prepare (sampleRate);
}

void ARKAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ARKAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::stereo())
        return false;

    const auto mainIn = layouts.getMainInputChannelSet();
    if (! mainIn.isDisabled() && mainIn != mainOut)
        return false;

    return true;
  #endif
}
#endif // ! JucePlugin_PreferredChannelConfigurations

void ARKAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Process MIDI messages
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Inject pitch bend and mod wheel from UI wheels
    int pb  = incomingPitchBend.load();
    int mod = incomingModWheel.load();

    juce::MidiBuffer extraMidi;
    extraMidi.addEvent(juce::MidiMessage::pitchWheel(1, pb),  0);
    extraMidi.addEvent(juce::MidiMessage::controllerEvent(1, 1, mod), 1);

    // Apply OCT/SEMI transpose to all note-on/note-off events
    int octShift  = keyboardOctaveOffset.load() * 12;
    int semiShift = keyboardSemiOffset.load();
    int totalShift = octShift + semiShift;

    if (totalShift != 0)
    {
        juce::MidiBuffer transposedMidi;
        for (const auto meta : midiMessages)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn() || msg.isNoteOff())
            {
                int newNote = juce::jlimit(0, 127, msg.getNoteNumber() + totalShift);
                if (msg.isNoteOn())
                    msg = juce::MidiMessage::noteOn (msg.getChannel(), newNote, msg.getVelocity());
                else
                    msg = juce::MidiMessage::noteOff(msg.getChannel(), newNote, msg.getVelocity());
            }
            transposedMidi.addEvent(msg, meta.samplePosition);
        }
        midiMessages.swapWith(transposedMidi);
    }

    // Merge in pitch bend and mod wheel
    for (const auto meta : extraMidi)
        midiMessages.addEvent(meta.getMessage(), meta.samplePosition);

    // MIDI Thru: save a copy of incoming messages BEFORE we consume them
    int midiThruOn = (int)apvts.getRawParameterValue("midiThru")->load();
    juce::MidiBuffer thruBuffer;
    if (midiThruOn == 1)
        thruBuffer = midiMessages;

    // Read voicing parameters once per block
    int  monoMode     = (int)apvts.getRawParameterValue("monoMode")->load();
    int  legatoOn     = (int)apvts.getRawParameterValue("legato")->load();
    float portaTime   = apvts.getRawParameterValue("portaTime")->load();
    float portaCurve  = apvts.getRawParameterValue("portaCurve")->load();
    int  portaAlways  = (int)apvts.getRawParameterValue("portaAlways")->load();
    int  maxVoicesCap = (int)apvts.getRawParameterValue("maxVoicesParam")->load();
    float veloCurve   = apvts.getRawParameterValue("veloCurve")->load();
    float noteTrackCrv = apvts.getRawParameterValue("noteTrackCurve")->load();

    // Convert porta knob (0-100) to milliseconds (0-5000ms)
    float portaMs = (portaTime / 100.0f) * 5000.0f;
    float portaSamples = (portaMs / 1000.0f) * (float)currentSampleRate;
    float portaIncrement = (portaSamples > 0.0f) ? (1.0f / portaSamples) : 1.0f;

    // Read ARP parameters once per block
    int  arpEnabled  = (int)apvts.getRawParameterValue("arpOnOff")->load();
    int  arpMode     = (int)apvts.getRawParameterValue("arpMode")->load();
    juce::ignoreUnused (arpMode);
    float arpRate    = apvts.getRawParameterValue("arpRate")->load();
    float arpGate    = apvts.getRawParameterValue("arpGate")->load();
    float arpSwingAmt = apvts.getRawParameterValue("arpSwing")->load();
    int  arpOctave   = (int)apvts.getRawParameterValue("arpOctave")->load();
    int  arpPattern  = (int)apvts.getRawParameterValue("arpPattern")->load();
    int  arpLatch    = (int)apvts.getRawParameterValue("arpLatch")->load();

    // When latch is toggled OFF, clear all held notes so the arp stops
    if (arpLatchPrev == 1 && arpLatch == 0)
        arpReset();
    arpLatchPrev = arpLatch;

    // Calculate ARP step duration from host tempo
    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        auto pos = playHead->getPosition();
        if (pos.hasValue() && pos->getBpm().hasValue())
            bpm = *pos->getBpm();
    }
    // arpRate: 1=whole, 2=half, 4=quarter, 8=eighth, 16=sixteenth
    float beatsPerStep = (arpRate > 0.0f) ? (4.0f / arpRate) : 1.0f;
    int arpStepSamples = (int)((60.0 / bpm) * beatsPerStep * currentSampleRate);
    if (arpStepSamples < 1) arpStepSamples = 1;

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        // ---- MIDI Channel Filter ----
        int midiChFilter = (int)apvts.getRawParameterValue("midiChannel")->load();
        if (midiChFilter > 0 && message.getChannel() != midiChFilter)
            continue; // ignore messages not on our channel

        // ---- Pitch Wheel ----
        if (message.isPitchWheel())
        {
            // JUCE pitch wheel: -8192 to +8191, normalize to -1.0 to +1.0
            currentPitchBend = juce::jlimit(-1.0f, 1.0f,
                ((float)message.getPitchWheelValue() - 8192.0f) / 8192.0f);
            continue;
        }

        // ---- Mod Wheel (CC1) ----
        if (message.isController() && message.getControllerNumber() == 1)
        {
            currentModWheel = message.getControllerValue() / 127.0f;
            continue;
        }
        if (arpEnabled)
        {
            if (message.isNoteOn())
            {
                int newNote = message.getNoteNumber();
                float rawVelocity = message.getVelocity() / 127.0f;
                arpLastVelocity = shapeVelocity(rawVelocity, veloCurve);
                arpPushNote(newNote);
                arpBuildSequence(arpPattern, arpOctave);
                if (arpHeldNoteCount == 1)
                {
                    arpCurrentStep = 0;
                    arpSamplesUntilNext = 0;
                    arpGoingUp = true;
                }
                continue; // skip normal note-on handling
            }
            else if (message.isNoteOff())
            {
                int offNote = message.getNoteNumber();
                if (arpLatch == 0)
                {
                    arpRemoveNote(offNote);
                    arpBuildSequence(arpPattern, arpOctave);
                    if (arpHeldNoteCount == 0)
                    {
                        if (arpNoteIsOn && arpCurrentNote >= 0)
                            arpNoteOff(arpCurrentNote);
                        arpCurrentStep = 0;
                        arpSamplesUntilNext = 0;
                    }
                    else if (arpCurrentStep >= arpSequenceLength)
                        arpCurrentStep = 0;
                }
                continue; // skip normal note-off handling
            }
        }

        if (message.isNoteOn())
        {
            int newNote = message.getNoteNumber();
            float rawVelocity = message.getVelocity() / 127.0f;
            float shapedVel = shapeVelocity(rawVelocity, veloCurve);
            float noteTrack = calculateNoteTracking(newNote, noteTrackCrv);
            float newFreq = 440.0f * std::pow(2.0f, (newNote - 69) / 12.0f);

            pushHeldNote(newNote);

            if (monoMode == 1)  // ---- MONO / LEGATO MODE ----
            {
                // Find the single active voice
                Voice* voice = nullptr;
                for (int i = 0; i < maxVoices; ++i)
                    if (voices[i].isActive) { voice = &voices[i]; break; }

                bool isLegato = (legatoOn == 1 && voice != nullptr &&
                                 voice->ampStage != Voice::EnvStage::Idle &&
                                 voice->ampStage != Voice::EnvStage::Release);

                if (voice == nullptr)
                    voice = &voices[0]; // use first voice for mono

                if (isLegato)
                {
                    // LEGATO: change pitch but do NOT retrigger envelopes
                    voice->midiNote = newNote;
                    voice->targetFrequency = newFreq;
                    voice->shapedVelocity = shapedVel;
                    voice->noteTrackValue = noteTrack;

                    if (portaTime > 0.01f)
                    {
                        voice->portaStartFrequency = voice->currentFrequency;
                        voice->portaProgress = 0.0f;
                    }
                    else
                    {
                        voice->currentFrequency = newFreq;
                        voice->portaProgress = 1.0f;
                    }
                }
                else
                {
                    // MONO (non-legato): retrigger everything
                    voice->isActive = true;
                    voice->midiNote = newNote;
                    voice->currentIndex = 0.0f;
                    voice->subIndex = 0.0f;
                    voice->level = shapedVel * 0.3f;
                    voice->shapedVelocity = shapedVel;
                    voice->noteTrackValue = noteTrack;
                    voice->voiceStartTime = ++voiceCounter;

                    // Portamento
                    voice->targetFrequency = newFreq;
                    if (portaTime > 0.01f && (portaAlways == 1 || voice->lastMidiNote >= 0))
                    {
                        if (voice->lastMidiNote >= 0)
                            voice->portaStartFrequency = 440.0f *
                                std::pow(2.0f, (voice->lastMidiNote - 69) / 12.0f);
                        else
                            voice->portaStartFrequency = newFreq;
                        voice->portaProgress = 0.0f;
                    }
                    else
                    {
                        voice->currentFrequency = newFreq;
                        voice->portaStartFrequency = newFreq;
                        voice->portaProgress = 1.0f;
                    }

                    // Retrigger all envelopes
                    voice->ampStage = Voice::EnvStage::Attack;
                    voice->ampEnvLevel = 0.0f;
                    voice->filterStage = Voice::EnvStage::Attack;
                    voice->filterEnvLevel = 0.0f;
                    voice->env3Stage = Voice::EnvStage::Attack;
                    voice->env3Level = 0.0f;

                    // Reset string synthesis state for new note (both oscillators)
                    voice->stringA.excitationEnergy = 0.0f;
                    voice->stringA.stringMode = 0;
                    voice->stringB.excitationEnergy = 0.0f;
                    voice->stringB.stringMode = 0;

                    // Reset choir synthesis state for new note (both oscillators)
                    voice->choirA.reset();
                    voice->choirB.reset();

                    // Reset LFOs based on trigger mode
                    for (int lfo = 0; lfo < 4; ++lfo)
                    {
                        const char* trigIds[] = {"lfo1TrigMode","lfo2TrigMode","lfo3TrigMode","lfo4TrigMode"};
                        int trigMode = (int)apvts.getRawParameterValue(trigIds[lfo])->load();
                        if (trigMode == 0)
                        {
                            voice->lfoPhase[lfo] = 0.0f;
                            voice->lfoValue[lfo] = 0.0f;
                            voice->lfoActive[lfo] = false;
                            voice->lfoRiseLevel[lfo] = 0.0f;
                            voice->lfoSmoothed[lfo] = 0.0f;
                            const char* delayIds[] = {"lfo1Delay","lfo2Delay","lfo3Delay","lfo4Delay"};
                            float delayMs = apvts.getRawParameterValue(delayIds[lfo])->load();
                            voice->lfoDelayTime[lfo] = (delayMs / 1000.0f) * (float)currentSampleRate;
                        }
                    }
                }

                voice->lastMidiNote = newNote;

                // Kill all other voices (mono = 1 voice only)
                for (int i = 0; i < maxVoices; ++i)
                    if (&voices[i] != voice && voices[i].isActive)
                        voices[i].isActive = false;
            }
            else  // ---- POLY MODE ----
            {
                Voice* voice = allocateVoice(maxVoicesCap);
                if (voice != nullptr)
                {
                    int prevNote = voice->lastMidiNote;

                    voice->isActive = true;
                    voice->midiNote = newNote;
                    voice->currentIndex = 0.0f;
                    voice->subIndex = 0.0f;
                    voice->level = shapedVel * 0.3f;
                    voice->shapedVelocity = shapedVel;
                    voice->noteTrackValue = noteTrack;
                    voice->voiceStartTime = ++voiceCounter;

                    // Portamento in poly mode
                    voice->targetFrequency = newFreq;
                    if (portaTime > 0.01f && portaAlways == 1 && prevNote >= 0)
                    {
                        voice->portaStartFrequency = 440.0f *
                            std::pow(2.0f, (prevNote - 69) / 12.0f);
                        voice->portaProgress = 0.0f;
                    }
                    else
                    {
                        voice->currentFrequency = newFreq;
                        voice->portaStartFrequency = newFreq;
                        voice->portaProgress = 1.0f;
                    }

                    // Retrigger envelopes
                    voice->ampStage = Voice::EnvStage::Attack;
                    voice->ampEnvLevel = 0.0f;
                    voice->filterStage = Voice::EnvStage::Attack;
                    voice->filterEnvLevel = 0.0f;
                    voice->env3Stage = Voice::EnvStage::Attack;
                    voice->env3Level = 0.0f;

                    // Reset string synthesis state for new note (both oscillators)
                    voice->stringA.excitationEnergy = 0.0f;
                    voice->stringA.stringMode = 0;
                    voice->stringB.excitationEnergy = 0.0f;
                    voice->stringB.stringMode = 0;

                    // Reset choir synthesis state for new note (both oscillators)
                    voice->choirA.reset();
                    voice->choirB.reset();

                    // LFO reset
                    for (int lfo = 0; lfo < 4; ++lfo)
                    {
                        const char* trigIds[] = {"lfo1TrigMode","lfo2TrigMode","lfo3TrigMode","lfo4TrigMode"};
                        int trigMode = (int)apvts.getRawParameterValue(trigIds[lfo])->load();
                        if (trigMode == 0)
                        {
                            voice->lfoPhase[lfo] = 0.0f;
                            voice->lfoValue[lfo] = 0.0f;
                            voice->lfoActive[lfo] = false;
                            voice->lfoRiseLevel[lfo] = 0.0f;
                            voice->lfoSmoothed[lfo] = 0.0f;
                            const char* delayIds[] = {"lfo1Delay","lfo2Delay","lfo3Delay","lfo4Delay"};
                            float delayMs = apvts.getRawParameterValue(delayIds[lfo])->load();
                            voice->lfoDelayTime[lfo] = (delayMs / 1000.0f) * (float)currentSampleRate;
                        }
                    }

                    voice->lastMidiNote = newNote;
                }
            }
        }
        else if (message.isNoteOff())
        {
            int offNote = message.getNoteNumber();
            removeHeldNote(offNote);

            if (monoMode == 1) // MONO/LEGATO
            {
                if (heldNoteCount > 0)
                {
                    // Notes still held - glide back to top of stack
                    int returnNote = topHeldNote();
                    float retFreq = 440.0f * std::pow(2.0f, (returnNote - 69) / 12.0f);
                    for (int i = 0; i < maxVoices; ++i)
                    {
                        if (voices[i].isActive)
                        {
                            voices[i].midiNote = returnNote;
                            voices[i].targetFrequency = retFreq;
                            voices[i].noteTrackValue = calculateNoteTracking(returnNote, noteTrackCrv);
                            if (portaTime > 0.01f)
                            {
                                voices[i].portaStartFrequency = voices[i].currentFrequency;
                                voices[i].portaProgress = 0.0f;
                            }
                            else
                            {
                                voices[i].currentFrequency = retFreq;
                                voices[i].portaProgress = 1.0f;
                            }
                            voices[i].lastMidiNote = returnNote;

                            // In legato mode, do NOT retrigger envelopes on note return
                            if (legatoOn == 0)
                            {
                                voices[i].ampStage = Voice::EnvStage::Attack;
                                voices[i].ampEnvLevel = 0.0f;
                                voices[i].filterStage = Voice::EnvStage::Attack;
                                voices[i].filterEnvLevel = 0.0f;
                                voices[i].env3Stage = Voice::EnvStage::Attack;
                                voices[i].env3Level = 0.0f;
                            }
                            break;
                        }
                    }
                }
                else
                {
                    // No notes held - release all
                    for (int i = 0; i < maxVoices; ++i)
                    {
                        if (voices[i].isActive)
                        {
                            voices[i].ampStage = Voice::EnvStage::Release;
                            voices[i].filterStage = Voice::EnvStage::Release;
                            voices[i].env3Stage = Voice::EnvStage::Release;
                        }
                    }
                }
            }
            else // POLY
            {
                // Release ALL voices playing this note, not just the first match.
                // With long release times, re-triggering the same note can create
                // multiple voices with the same midiNote — only releasing the first
                // one leaves the others stuck forever.
                for (int i = 0; i < maxVoices; ++i)
                {
                    if (voices[i].isActive && voices[i].midiNote == offNote
                        && voices[i].ampStage != Voice::EnvStage::Release)
                    {
                        voices[i].ampStage = Voice::EnvStage::Release;
                        voices[i].filterStage = Voice::EnvStage::Release;
                        voices[i].env3Stage = Voice::EnvStage::Release;
                    }
                }
            }
        }
    }


    // Generate audio by mixing all active voices
    buffer.clear();

    // Read waveform choices and power from APVTS
    auto* oscAParam = apvts.getRawParameterValue ("oscAWave");
    auto* oscBParam = apvts.getRawParameterValue ("oscBWave");
    auto* oscAStringModeParam = apvts.getRawParameterValue ("oscAStringMode");
    auto* oscBStringModeParam = apvts.getRawParameterValue ("oscBStringMode");
    auto* oscAChoirModeParam  = apvts.getRawParameterValue ("oscAChoirMode");
    auto* oscBChoirModeParam  = apvts.getRawParameterValue ("oscBChoirMode");
    auto* oscAPowerParam = apvts.getRawParameterValue ("oscAPower");
    auto* oscBPowerParam = apvts.getRawParameterValue ("oscBPower");
    
    // Read OSC A parameters
    auto* oscAOctaveParam = apvts.getRawParameterValue ("oscAOctave");
    auto* oscASemitoneParam = apvts.getRawParameterValue ("oscASemitone");
    auto* oscAFineParam = apvts.getRawParameterValue ("oscAFine");
    auto* oscAUnisonParam = apvts.getRawParameterValue ("oscAUnison");
    auto* oscADetuneParam = apvts.getRawParameterValue ("oscADetune");
    auto* oscABlendParam = apvts.getRawParameterValue ("oscABlend");
    auto* oscAWtPosParam = apvts.getRawParameterValue ("oscAWtPos");
    auto* oscASpreadParam = apvts.getRawParameterValue ("oscASpread");
    auto* oscAChaosParam = apvts.getRawParameterValue ("oscAChaos");
    auto* oscALevelParam = apvts.getRawParameterValue ("oscALevel");
    auto* oscAPanParam = apvts.getRawParameterValue ("oscAPan");
    auto* oscAPhaseParam = apvts.getRawParameterValue ("oscAPhase");
    
    // Read OSC B parameters
    auto* oscBOctaveParam = apvts.getRawParameterValue ("oscBOctave");
    auto* oscBSemitoneParam = apvts.getRawParameterValue ("oscBSemitone");
    auto* oscBFineParam = apvts.getRawParameterValue ("oscBFine");
    auto* oscBUnisonParam = apvts.getRawParameterValue ("oscBUnison");
    auto* oscBDetuneParam = apvts.getRawParameterValue ("oscBDetune");
    auto* oscBBlendParam = apvts.getRawParameterValue ("oscBBlend");
    auto* oscBWtPosParam = apvts.getRawParameterValue ("oscBWtPos");
    auto* oscBSpreadParam = apvts.getRawParameterValue ("oscBSpread");
    auto* oscBChaosParam = apvts.getRawParameterValue ("oscBChaos");
    auto* oscBLevelParam = apvts.getRawParameterValue ("oscBLevel");
    auto* oscBPanParam = apvts.getRawParameterValue ("oscBPan");
    auto* oscBPhaseParam = apvts.getRawParameterValue ("oscBPhase");
    
    // Safety check
    if (oscAParam == nullptr || oscBParam == nullptr ||
        oscAStringModeParam == nullptr || oscBStringModeParam == nullptr ||
        oscAChoirModeParam == nullptr || oscBChoirModeParam == nullptr ||
        oscAPowerParam == nullptr || oscBPowerParam == nullptr ||
        oscAOctaveParam == nullptr || oscASemitoneParam == nullptr || oscAFineParam == nullptr ||
        oscAUnisonParam == nullptr || oscADetuneParam == nullptr || oscABlendParam == nullptr ||
        oscAWtPosParam == nullptr || oscASpreadParam == nullptr || oscAChaosParam == nullptr ||
        oscALevelParam == nullptr || oscAPanParam == nullptr || oscAPhaseParam == nullptr ||
        oscBOctaveParam == nullptr || oscBSemitoneParam == nullptr || oscBFineParam == nullptr ||
        oscBUnisonParam == nullptr || oscBDetuneParam == nullptr || oscBBlendParam == nullptr ||
        oscBWtPosParam == nullptr || oscBSpreadParam == nullptr || oscBChaosParam == nullptr ||
        oscBLevelParam == nullptr || oscBPanParam == nullptr || oscBPhaseParam == nullptr)
        return; // Safety check
    
    int oscAWave = (int) oscAParam->load();
    int oscBWave = (int) oscBParam->load();
    int oscAStringMode = (int) oscAStringModeParam->load();  // 0=None, 1=Pluck, 2=Strum, 3=Pizz, 4=Arco
    int oscBStringMode = (int) oscBStringModeParam->load();  // 0=None, 1=Pluck, 2=Strum, 3=Pizz, 4=Arco
    int oscAChoirMode  = (int) oscAChoirModeParam->load();   // 0=None, 1=OOH, 2=AAH, 3=Women, 4=Men
    int oscBChoirMode  = (int) oscBChoirModeParam->load();   // 0=None, 1=OOH, 2=AAH, 3=Women, 4=Men
    bool oscAPowerOn = oscAPowerParam->load() > 0.5f;
    bool oscBPowerOn = oscBPowerParam->load() > 0.5f;
    
    float oscAOctave = oscAOctaveParam->load();
    float oscASemitone = oscASemitoneParam->load();
    float oscAFine = oscAFineParam->load();
    int oscAUnison = (int) oscAUnisonParam->load();
    float oscADetune = oscADetuneParam->load();
    float oscABlend = oscABlendParam->load();
    float oscAWtPos = oscAWtPosParam->load();
    float oscASpread = oscASpreadParam->load();
    float oscAChaos = oscAChaosParam->load();
    float oscALevel = oscALevelParam->load();
    float oscAPan = oscAPanParam->load();
    float oscAPhase = oscAPhaseParam->load();
    
    float oscBOctave = oscBOctaveParam->load();
    float oscBSemitone = oscBSemitoneParam->load();
    float oscBFine = oscBFineParam->load();
    int oscBUnison = (int) oscBUnisonParam->load();
    float oscBDetune = oscBDetuneParam->load();
    float oscBBlend = oscBBlendParam->load();
    float oscBWtPos = oscBWtPosParam->load();
    float oscBSpread = oscBSpreadParam->load();
    float oscBChaos = oscBChaosParam->load();
    float oscBLevel = oscBLevelParam->load();
    float oscBPan = oscBPanParam->load();
    float oscBPhase = oscBPhaseParam->load();

    // -------------------------------------------------------------------------
    // Read and apply Filter parameters
    // -------------------------------------------------------------------------
    auto* filterCutoffParam    = apvts.getRawParameterValue ("filterCutoff");
    auto* filterResonanceParam = apvts.getRawParameterValue ("filterResonance");
    auto* filterDriveParam     = apvts.getRawParameterValue ("filterDrive");
    auto* filterModeParam      = apvts.getRawParameterValue ("filterMode");
    auto* filterCharacterParam = apvts.getRawParameterValue ("filterCharacter");

    // Store base filter values ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â filter envelope will modulate cutoff per-sample
    float baseCutoffHz = filterCutoffParam ? filterCutoffParam->load() : 20000.0f;
    float baseRes      = filterResonanceParam ? filterResonanceParam->load() / 100.0f : 0.0f;
    float baseDrive    = 1.0f + (filterDriveParam ? filterDriveParam->load() / 100.0f : 0.0f) * 9.0f;
    int   filterMode      = filterModeParam      ? (int)filterModeParam->load()      : 0;
    int   filterCharacter = filterCharacterParam ? (int)filterCharacterParam->load() : 0;

    filterLeft.mode           = filterMode;
    filterRight.mode          = filterMode;
    filterLeft.characterMode  = filterCharacter;
    filterRight.characterMode = filterCharacter;

    // -------------------------------------------------------------------------
    // Read Sub Oscillator parameters
    // -------------------------------------------------------------------------
    auto* subPowerParam  = apvts.getRawParameterValue ("subPower");
    auto* subWaveParam   = apvts.getRawParameterValue ("subWave");
    auto* subOctaveParam = apvts.getRawParameterValue ("subOctave");
    auto* subLevelParam  = apvts.getRawParameterValue ("subLevel");
    auto* subPanParam    = apvts.getRawParameterValue ("subPan");

    bool  subPowerOn = (subPowerParam  != nullptr) ? subPowerParam->load()  > 0.5f : false;
    int   subWave    = (subWaveParam   != nullptr) ? (int)subWaveParam->load()     : 0;
    float subOctave  = (subOctaveParam != nullptr) ? subOctaveParam->load()        : -1.0f;
    float subLevel   = (subLevelParam  != nullptr) ? subLevelParam->load()         : 75.0f;
    float subPan     = (subPanParam    != nullptr) ? subPanParam->load()           : 0.0f;

    // -------------------------------------------------------------------------
    // Read Noise Generator parameters
    // -------------------------------------------------------------------------
    auto* noisePowerParam = apvts.getRawParameterValue ("noisePower");
    auto* noiseTypeParam  = apvts.getRawParameterValue ("noiseType");
    auto* noiseLevelParam = apvts.getRawParameterValue ("noiseLevel");
    auto* noisePanParam   = apvts.getRawParameterValue ("noisePan");
    auto* noiseGateParam  = apvts.getRawParameterValue ("noiseGate");
    auto* noiseCutoffParam    = apvts.getRawParameterValue ("noiseCutoff");
    auto* noiseLfoAmtParam    = apvts.getRawParameterValue ("noiseLfoAmount");

    bool  noisePowerOn = (noisePowerParam != nullptr) ? noisePowerParam->load() > 0.5f : false;
    int   noiseType    = (noiseTypeParam  != nullptr) ? (int)noiseTypeParam->load()    : 0;
    float noiseLevel   = (noiseLevelParam != nullptr) ? noiseLevelParam->load()        : 0.0f;
    float noisePan     = (noisePanParam   != nullptr) ? noisePanParam->load()          : 0.0f;
    bool  noiseKeyMode = (noiseGateParam  != nullptr) ? noiseGateParam->load() > 0.5f : false;
    float noiseCutoffHz  = (noiseCutoffParam  != nullptr) ? noiseCutoffParam->load()  : 20000.0f;
    float noiseLfoAmt    = (noiseLfoAmtParam  != nullptr) ? noiseLfoAmtParam->load()  : 0.0f;

    // In KEY mode check if any voice is currently active
    bool anyVoiceActive = false;
    float noiseEnvGain = 0.0f;  // ADD THIS
    if (noiseKeyMode)
    {
        int activeCount = 0;
        for (int v = 0; v < maxVoices; ++v)
        {
            if (voices[v].isActive)
            {
                anyVoiceActive = true;
                noiseEnvGain += voices[v].ampEnvLevel;  // ADD THIS
                ++activeCount;                           // ADD THIS
            }
        }
        if (activeCount > 0)                             // ADD THIS
            noiseEnvGain /= (float)activeCount;          // ADD THIS
    }
    else
    {
        noiseEnvGain = 1.0f;  // FREE mode: always full level
    }

    // -------------------------------------------------------------------------
    // Read ENV 1 (Amplitude) and ENV 2 (Filter) parameters
    // Convert ms to per-sample smoothing coefficient
    // -------------------------------------------------------------------------
    auto* env1AtkParam = apvts.getRawParameterValue ("env1Attack");
    auto* env1DcyParam = apvts.getRawParameterValue ("env1Decay");
    auto* env1SusParam = apvts.getRawParameterValue ("env1Sustain");
    auto* env1RelParam = apvts.getRawParameterValue ("env1Release");

    auto* fEnvAtkParam = apvts.getRawParameterValue ("filterEnvAttack");
    auto* fEnvDcyParam = apvts.getRawParameterValue ("filterEnvDecay");
    auto* fEnvSusParam = apvts.getRawParameterValue ("filterEnvSustain");
    auto* fEnvRelParam = apvts.getRawParameterValue ("filterEnvRelease");
    auto* fEnvAmtParam = apvts.getRawParameterValue ("filterEnvAmount");

    // ms ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ per-sample coefficient (avoids division by zero)
    auto msToCoeff = [&](float ms) -> float
    {
        if (ms < 1.0f) return 1.0f;
        return 1.0f - std::exp (-1.0f / (ms * (float)currentSampleRate / 1000.0f));
    };

    float env1AtkCoeff = msToCoeff (env1AtkParam ? env1AtkParam->load() : 1.0f);
    float env1DcyCoeff = msToCoeff (env1DcyParam ? env1DcyParam->load() : 295.0f);
    float env1SusLevel = (env1SusParam ? env1SusParam->load() : 70.0f) / 100.0f;
    float env1RelCoeff = msToCoeff (env1RelParam ? env1RelParam->load() : 117.0f);

    float fEnvAtkCoeff = msToCoeff (fEnvAtkParam ? fEnvAtkParam->load() : 10.0f);
    float fEnvDcyCoeff = msToCoeff (fEnvDcyParam ? fEnvDcyParam->load() : 200.0f);
    float fEnvSusLevel = (fEnvSusParam ? fEnvSusParam->load() : 70.0f) / 100.0f;
    float fEnvRelCoeff = msToCoeff (fEnvRelParam ? fEnvRelParam->load() : 300.0f);
    float fEnvAmount   = (fEnvAmtParam ? fEnvAmtParam->load() : 0.0f) / 100.0f; // -1 to +1

    // -------------------------------------------------------------------------
    // Read ENV 3 (Assignable Envelope) parameters
    // -------------------------------------------------------------------------
    auto* env3AtkParam  = apvts.getRawParameterValue ("env3Attack");
    auto* env3DcyParam  = apvts.getRawParameterValue ("env3Decay");
    auto* env3SusParam  = apvts.getRawParameterValue ("env3Sustain");
    auto* env3RelParam  = apvts.getRawParameterValue ("env3Release");
    auto* env3AmtParam  = apvts.getRawParameterValue ("env3Amount");
    auto* env3DestParam = apvts.getRawParameterValue ("env3Destination");
    
    float env3AtkCoeff = msToCoeff (env3AtkParam ? env3AtkParam->load() : 10.0f);
    float env3DcyCoeff = msToCoeff (env3DcyParam ? env3DcyParam->load() : 200.0f);
    float env3SusLevel = (env3SusParam ? env3SusParam->load() : 70.0f) / 100.0f;
    float env3RelCoeff = msToCoeff (env3RelParam ? env3RelParam->load() : 300.0f);
    float env3Amount   = (env3AmtParam ? env3AmtParam->load() : 0.0f) / 100.0f;
    int   env3Dest     = env3DestParam ? (int)env3DestParam->load() : 0;

    // -------------------------------------------------------------------------
    // Read LFO parameters once per block (NOT per sample - too expensive!)
    // -------------------------------------------------------------------------
    const int lfoWave1 = (int)apvts.getRawParameterValue("lfo1Wave")->load();
    const float lfoRate1 = apvts.getRawParameterValue("lfo1Rate")->load();
    const float lfoRise1 = apvts.getRawParameterValue("lfo1Rise")->load();
    const float lfoDelay1 = apvts.getRawParameterValue("lfo1Delay")->load();
    const float lfoSmooth1 = apvts.getRawParameterValue("lfo1Smooth")->load();

    const int lfoWave2 = (int)apvts.getRawParameterValue("lfo2Wave")->load();
    const float lfoRate2 = apvts.getRawParameterValue("lfo2Rate")->load();
    const float lfoRise2 = apvts.getRawParameterValue("lfo2Rise")->load();
    const float lfoDelay2 = apvts.getRawParameterValue("lfo2Delay")->load();
    const float lfoSmooth2 = apvts.getRawParameterValue("lfo2Smooth")->load();

    const int lfoWave3 = (int)apvts.getRawParameterValue("lfo3Wave")->load();
    const float lfoRate3 = apvts.getRawParameterValue("lfo3Rate")->load();
    const float lfoRise3 = apvts.getRawParameterValue("lfo3Rise")->load();
    const float lfoDelay3 = apvts.getRawParameterValue("lfo3Delay")->load();
    const float lfoSmooth3 = apvts.getRawParameterValue("lfo3Smooth")->load();

    const int lfoWave4 = (int)apvts.getRawParameterValue("lfo4Wave")->load();
    const float lfoRate4 = apvts.getRawParameterValue("lfo4Rate")->load();
    const float lfoRise4 = apvts.getRawParameterValue("lfo4Rise")->load();
    const float lfoDelay4 = apvts.getRawParameterValue("lfo4Delay")->load();
    const float lfoSmooth4 = apvts.getRawParameterValue("lfo4Smooth")->load();

    // LFO trigger modes: 0=TRIG, 1=FREE, 2=OFF
    const int lfoTrigMode1 = (int)apvts.getRawParameterValue("lfo1TrigMode")->load();
    const int lfoTrigMode2 = (int)apvts.getRawParameterValue("lfo2TrigMode")->load();
    const int lfoTrigMode3 = (int)apvts.getRawParameterValue("lfo3TrigMode")->load();
    const int lfoTrigMode4 = (int)apvts.getRawParameterValue("lfo4TrigMode")->load();

    // Filter LFO amount - read once per block
    auto* fLfoAmtParam = apvts.getRawParameterValue ("filterLfoAmount");
    float fLfoAmount = (fLfoAmtParam ? fLfoAmtParam->load() : 0.0f) / 100.0f; // -1 to +1

    // -------------------------------------------------------------------------
    // Read MODULATION MATRIX parameters once per block
    // -------------------------------------------------------------------------
    struct MatrixSlot { int source; int dest; float amount; bool enabled; };
    MatrixSlot matrixSlots[16];
    for (int i = 0; i < 16; ++i)
    {
        juce::String idx = juce::String(i + 1);
        matrixSlots[i].source  = (int)apvts.getRawParameterValue("matrixSource"  + idx)->load();
        matrixSlots[i].dest    = (int)apvts.getRawParameterValue("matrixDest"    + idx)->load();
        matrixSlots[i].amount  = apvts.getRawParameterValue("matrixAmount" + idx)->load() / 100.0f;
        matrixSlots[i].enabled = apvts.getRawParameterValue("matrixEnabled" + idx)->load() > 0.5f;
    }

    // -------------------------------------------------------------------------
    // Hoist parameter reads that are constant across all samples in this block
    // -------------------------------------------------------------------------
    const int   modWheelDest     = (int)apvts.getRawParameterValue("modWheelDest")->load();
    const float bendUp           = (float)(int)apvts.getRawParameterValue("pitchBendUp")->load();
    const float bendDown         = (float)(int)apvts.getRawParameterValue("pitchBendDown")->load();
    const float masterTuneCents  = apvts.getRawParameterValue("masterTune")->load();
    const float masterTranspSemi = (float)(int)apvts.getRawParameterValue("masterTranspose")->load();
    const float masterVol        = apvts.getRawParameterValue("masterVolume")->load() / 100.0f;
    const float masterPanBase    = apvts.getRawParameterValue("masterPan")->load();

    // MOD section parameters
    const float modFattness = apvts.getRawParameterValue("modFattness")->load();  // 0–10
    const float modHpFilter = apvts.getRawParameterValue("modHpFilter")->load();  // 0–10
    const float modFm       = apvts.getRawParameterValue("modFm")->load();        // 0–100
    const float modFmFilter = apvts.getRawParameterValue("modFmFilter")->load();  // 0–100
    const float modTighten  = apvts.getRawParameterValue("modTighten")->load();   // 0–10
    const float modHuman    = apvts.getRawParameterValue("modHuman")->load();     // 0–10

    // HP filter coefficient: maps 0–10 → ~20–800 Hz cutoff
    const float hpCutoffHz = 20.0f * std::pow(40.0f, modHpFilter / 10.0f);
    const float hpAlpha    = 1.0f / (1.0f + juce::MathConstants<float>::twoPi
                             * hpCutoffHz / (float)currentSampleRate);

    // Pre-compute pitch bend in semitones (constant for this block)
    const float bendSemitones = (currentPitchBend >= 0.0f)
                                ? currentPitchBend * bendUp
                                : currentPitchBend * bendDown;

    // Mod wheel pitch contribution
    const float modWheelPitch = (modWheelDest == 3) ? currentModWheel * 2.0f : 0.0f;

    // Combined global pitch offset for all voices
    const float globalPitchOffset = bendSemitones + masterTranspSemi + modWheelPitch
                                    + (masterTuneCents / 100.0f);

    // Pre-compute OSC panning gains (constant for block, avoids sqrt per voice per sample)
    const float panANorm = (oscAPan + 100.0f) / 200.0f;
    const float leftGainA  = std::sqrt(1.0f - panANorm);
    const float rightGainA = std::sqrt(panANorm);

    const float panBNorm = (oscBPan + 100.0f) / 200.0f;
    const float leftGainB  = std::sqrt(1.0f - panBNorm);
    const float rightGainB = std::sqrt(panBNorm);

    const float subPanNorm    = (subPan + 100.0f) / 200.0f;
    const float subLeftGain   = std::sqrt(1.0f - subPanNorm);
    const float subRightGain  = std::sqrt(subPanNorm);

    const float noisePanNorm    = (noisePan + 100.0f) / 200.0f;
    const float noiseLeftGain   = std::sqrt(1.0f - noisePanNorm);
    const float noiseRightGain  = std::sqrt(noisePanNorm);

    // Pre-compute unison spread stereo gains for OSC A
    float oscAUnisonLeftGains[16];
    float oscAUnisonRightGains[16];
    float oscAUnisonVoiceGains[16];
    float oscAUnisonSpreadPos[16];
    {
        float spreadAmt = oscASpread / 100.0f;
        for (int u = 0; u < oscAUnison; ++u)
        {
            float spreadPosition = 0.0f;
            if (oscAUnison > 1)
                spreadPosition = (float)u / (float)(oscAUnison - 1) * 2.0f - 1.0f;
            oscAUnisonSpreadPos[u] = spreadPosition;

            float stereoPos = spreadPosition * spreadAmt;
            oscAUnisonLeftGains[u]  = std::sqrt(0.5f - stereoPos * 0.5f);
            oscAUnisonRightGains[u] = std::sqrt(0.5f + stereoPos * 0.5f);

            float vGain = 1.0f / (float)oscAUnison;
            if (u == oscAUnison / 2 && oscAUnison > 1)
            {
                float centerBlend = (100.0f - oscABlend) / 50.0f;
                vGain *= juce::jlimit(0.5f, 2.0f, centerBlend);
            }
            oscAUnisonVoiceGains[u] = vGain;
        }
    }

    // Pre-compute unison spread stereo gains for OSC B
    float oscBUnisonLeftGains[16];
    float oscBUnisonRightGains[16];
    float oscBUnisonVoiceGains[16];
    float oscBUnisonSpreadPos[16];
    {
        float spreadAmt = oscBSpread / 100.0f;
        for (int u = 0; u < oscBUnison; ++u)
        {
            float spreadPosition = 0.0f;
            if (oscBUnison > 1)
                spreadPosition = (float)u / (float)(oscBUnison - 1) * 2.0f - 1.0f;
            oscBUnisonSpreadPos[u] = spreadPosition;

            float stereoPos = spreadPosition * spreadAmt;
            oscBUnisonLeftGains[u]  = std::sqrt(0.5f - stereoPos * 0.5f);
            oscBUnisonRightGains[u] = std::sqrt(0.5f + stereoPos * 0.5f);

            float vGain = 1.0f / (float)oscBUnison;
            if (u == oscBUnison / 2 && oscBUnison > 1)
            {
                float centerBlend = (100.0f - oscBBlend) / 50.0f;
                vGain *= juce::jlimit(0.5f, 2.0f, centerBlend);
            }
            oscBUnisonVoiceGains[u] = vGain;
        }
    }

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float mixedSampleLeft = 0.0f;
        float mixedSampleRight = 0.0f;

        // Accumulators for per-voice averages (computed in-line, avoids extra loops)
        int   currentActiveVoices = 0;
        float sumFilterEnvLevel = 0.0f;
        float sumLfo0           = 0.0f;
        float sumNoteTrack      = 0.0f;
        float sumEnv3           = 0.0f;
        float sumLfo[4]         = { 0.0f, 0.0f, 0.0f, 0.0f };
        float sumAmpEnv         = 0.0f;
        float sumFilterEnv      = 0.0f;
        float sumEnv3Mat        = 0.0f;
        float sumVel            = 0.0f;
        float sumNote           = 0.0f;

        // ---- ARP Per-Sample Stepping ----
        if (arpEnabled && arpSequenceLength > 0)
        {
            // Handle gate-off (note release within step)
            if (arpNoteIsOn && arpSamplesUntilGateOff > 0)
            {
                arpSamplesUntilGateOff--;
                if (arpSamplesUntilGateOff <= 0)
                {
                    if (arpCurrentNote >= 0)
                        arpNoteOff(arpCurrentNote);
                }
            }

            // Handle step advance
            if (arpSamplesUntilNext > 0)
                arpSamplesUntilNext--;

            if (arpSamplesUntilNext <= 0)
            {
                // Turn off previous note if still on
                if (arpNoteIsOn && arpCurrentNote >= 0)
                    arpNoteOff(arpCurrentNote);

                // Advance step
                int nextNote = -1;
                if (arpPattern == 3) // Random
                {
                    int randIdx = arpRandom.nextInt(arpSequenceLength);
                    nextNote = arpSequence[randIdx];
                }
                else
                {
                    if (arpCurrentStep >= arpSequenceLength)
                        arpCurrentStep = 0;
                    nextNote = arpSequence[arpCurrentStep];
                    arpCurrentStep++;
                    if (arpCurrentStep >= arpSequenceLength)
                        arpCurrentStep = 0;
                }

                // Clamp to MIDI range
                if (nextNote >= 0 && nextNote <= 127)
                {
                    arpNoteOn(nextNote, arpLastVelocity);
                }

                // Calculate next step duration with swing
                int thisSamples = arpStepSamples;
                // Swing: odd steps are delayed (lengthened), even steps shortened
                if (arpSwingAmt > 0.01f)
                {
                    float swingFactor = arpSwingAmt / 100.0f; // 0 to 1
                    bool isOddStep = (arpCurrentStep % 2 != 0);
                    if (isOddStep)
                        thisSamples = (int)(arpStepSamples * (1.0f + swingFactor * 0.5f));
                    else
                        thisSamples = (int)(arpStepSamples * (1.0f - swingFactor * 0.5f));
                    if (thisSamples < 1) thisSamples = 1;
                }

                arpSamplesUntilNext = thisSamples;

                // Calculate gate-off time
                float gateRatio = juce::jlimit(0.01f, 1.0f, arpGate / 100.0f);
                arpSamplesUntilGateOff = (int)(thisSamples * gateRatio);
                if (arpSamplesUntilGateOff < 1) arpSamplesUntilGateOff = 1;
            }
        }


        // Mix all active voices ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â blend OSC A and OSC B equally
        for (int v = 0; v < maxVoices; ++v)
        {
            if (voices[v].isActive)
            {
                // ----------------------------------------------------------
                // Update envelopes FIRST (before generating audio)
                // ----------------------------------------------------------

                // ENV 1 ÃƒÆ’Ã‚Â¢Ãƒâ€šÃ¢â€šÂ¬Ãƒâ€šÃ¢â‚¬Â Amplitude Envelope
                switch (voices[v].ampStage)
                {
                    case Voice::EnvStage::Attack:
                        voices[v].ampEnvLevel += env1AtkCoeff * (1.0f - voices[v].ampEnvLevel);
                        if (voices[v].ampEnvLevel >= 0.999f)
                        {
                            voices[v].ampEnvLevel = 1.0f;
                            voices[v].ampStage = Voice::EnvStage::Decay;
                        }
                        break;
                    case Voice::EnvStage::Decay:
                        voices[v].ampEnvLevel += env1DcyCoeff * (env1SusLevel - voices[v].ampEnvLevel);
                        if (std::abs (voices[v].ampEnvLevel - env1SusLevel) < 0.001f)
                            voices[v].ampStage = Voice::EnvStage::Sustain;
                        break;
                    case Voice::EnvStage::Sustain:
                        voices[v].ampEnvLevel = env1SusLevel;
                        break;
                    case Voice::EnvStage::Release:
                        voices[v].ampEnvLevel += env1RelCoeff * (0.0f - voices[v].ampEnvLevel);
                        if (voices[v].ampEnvLevel < 0.001f)
                        {
                            voices[v].ampEnvLevel = 0.0f;
                            voices[v].ampStage    = Voice::EnvStage::Idle;
                            voices[v].isActive    = false;
                            voices[v].midiNote    = -1;
                        }
                        break;
                    case Voice::EnvStage::Idle:
                        break;
                }

                // ENV 2 ÃƒÆ’Ã‚Â¢Ãƒâ€šÃ¢â€šÂ¬Ãƒâ€šÃ¢â‚¬Â Filter Envelope
                switch (voices[v].filterStage)
                {
                    case Voice::EnvStage::Attack:
                        voices[v].filterEnvLevel += fEnvAtkCoeff * (1.0f - voices[v].filterEnvLevel);
                        if (voices[v].filterEnvLevel >= 0.999f)
                        {
                            voices[v].filterEnvLevel = 1.0f;
                            voices[v].filterStage = Voice::EnvStage::Decay;
                        }
                        break;
                    case Voice::EnvStage::Decay:
                        voices[v].filterEnvLevel += fEnvDcyCoeff * (fEnvSusLevel - voices[v].filterEnvLevel);
                        if (std::abs (voices[v].filterEnvLevel - fEnvSusLevel) < 0.001f)
                            voices[v].filterStage = Voice::EnvStage::Sustain;
                        break;
                    case Voice::EnvStage::Sustain:
                        voices[v].filterEnvLevel = fEnvSusLevel;
                        break;
                    case Voice::EnvStage::Release:
                        voices[v].filterEnvLevel += fEnvRelCoeff * (0.0f - voices[v].filterEnvLevel);
                        if (voices[v].filterEnvLevel < 0.0001f)
                        {
                            voices[v].filterEnvLevel = 0.0f;
                            voices[v].filterStage    = Voice::EnvStage::Idle;
                        }
                        break;
                    case Voice::EnvStage::Idle:
                        break;
                }

                // ENV 3 ÃƒÆ’Ã‚Â¢Ãƒâ€šÃ¢â€šÂ¬Ãƒâ€šÃ¢â‚¬Â Assignable Modulation Envelope
                switch (voices[v].env3Stage)
                {
                    case Voice::EnvStage::Attack:
                        voices[v].env3Level += env3AtkCoeff * (1.0f - voices[v].env3Level);
                        if (voices[v].env3Level >= 0.999f)
                        {
                            voices[v].env3Level = 1.0f;
                            voices[v].env3Stage = Voice::EnvStage::Decay;
                        }
                        break;
                    case Voice::EnvStage::Decay:
                        voices[v].env3Level += env3DcyCoeff * (env3SusLevel - voices[v].env3Level);
                        if (std::abs (voices[v].env3Level - env3SusLevel) < 0.001f)
                            voices[v].env3Stage = Voice::EnvStage::Sustain;
                        break;
                    case Voice::EnvStage::Sustain:
                        voices[v].env3Level = env3SusLevel;
                        break;
                    case Voice::EnvStage::Release:
                        voices[v].env3Level += env3RelCoeff * (0.0f - voices[v].env3Level);
                        if (voices[v].env3Level < 0.0001f)
                        {
                            voices[v].env3Level = 0.0f;
                            voices[v].env3Stage = Voice::EnvStage::Idle;
                        }
                        break;
                    case Voice::EnvStage::Idle:
                        break;
                }

                updateLFO(voices[v], 0, lfoRate1, lfoRise1, lfoDelay1, lfoSmooth1, lfoWave1, lfoTrigMode1);
                updateLFO(voices[v], 1, lfoRate2, lfoRise2, lfoDelay2, lfoSmooth2, lfoWave2, lfoTrigMode2);
                updateLFO(voices[v], 2, lfoRate3, lfoRise3, lfoDelay3, lfoSmooth3, lfoWave3, lfoTrigMode3);
                updateLFO(voices[v], 3, lfoRate4, lfoRise4, lfoDelay4, lfoSmooth4, lfoWave4, lfoTrigMode4);

                // If voice just became inactive from release finishing, skip audio gen
                if (!voices[v].isActive)
                    continue;

                // ----------------------------------------------------------
                // Portamento: interpolate frequency toward target
                // ----------------------------------------------------------
                if (voices[v].portaProgress < 1.0f)
                {
                    float increment = portaIncrement;

                    // SCALED mode: adjust glide speed proportional to interval
                    if (portaAlways == 0 && voices[v].portaStartFrequency > 0.0f)
                    {
                        float ratio = voices[v].targetFrequency / voices[v].portaStartFrequency;
                        float semitoneDistance = std::abs(12.0f * std::log2(std::max(ratio, 0.001f)));
                        float scaleFactor = juce::jmax(0.083f, semitoneDistance / 12.0f);
                        increment = portaIncrement / scaleFactor;
                    }

                    voices[v].portaProgress += increment;
                    if (voices[v].portaProgress >= 1.0f)
                    {
                        voices[v].portaProgress = 1.0f;
                        voices[v].currentFrequency = voices[v].targetFrequency;
                    }
                    else
                    {
                        // Apply curve shaping and interpolate in log-frequency space
                        float shaped = applyPortaCurve(voices[v].portaProgress, portaCurve);
                        float startLog = std::log2(std::max(voices[v].portaStartFrequency, 1.0f));
                        float endLog   = std::log2(std::max(voices[v].targetFrequency, 1.0f));
                        float interpLog = startLog + shaped * (endLog - startLog);
                        voices[v].currentFrequency = std::pow(2.0f, interpLog);
                    }
                }

                // ----------------------------------------------------------
                // Now generate audio for this voice
                // ----------------------------------------------------------

                // Apply ENV 3 pitch modulation (per-voice)
                float voicePitchMod = (env3Dest == 1) ? voices[v].env3Level * env3Amount * 12.0f : 0.0f;

                // Apply ENV 3 pan modulation (per-voice)
                float voicePanMod = (env3Dest == 2) ? voices[v].env3Level * env3Amount : 0.0f;

                // Generate unison voices for OSC A
                float sampleALeft = 0.0f;
                float sampleARight = 0.0f;
                if (oscAPowerOn)
                {
                    for (int u = 0; u < oscAUnison; ++u)
                    {
                        float detuneOffset = (oscAUnison > 1)
                            ? oscAUnisonSpreadPos[u] * oscADetune : 0.0f;

                        // Apply CHAOS - add random variation to each voice
                        if (oscAChaos > 0.1f && oscAUnison > 1)
                        {
                            float randomSeed = (float)(v * 1000 + u * 137 + sample % 100);
                            float randomValue = std::sin(randomSeed) * 0.5f + 0.5f;

                            float chaosAmount = oscAChaos / 100.0f;
                            float chaosPitch = (randomValue - 0.5f) * 2.0f * chaosAmount * 50.0f;
                            detuneOffset += chaosPitch;
                        }

                        // FM: use OSC B's current phase value to modulate OSC A's pitch
                        float fmMod = 0.0f;
                        if (modFm > 0.0f && oscBPowerOn)
                            fmMod = std::sin(voices[v].currentIndex / (float)wavetableSize
                                             * juce::MathConstants<float>::twoPi) * (modFm / 100.0f) * 12.0f;

                        float unisonSample;

                        // Synthesis priority: String > Choir > Wavetable
                        if (oscAStringMode > 0)
                        {
                            float pitchShift = oscAOctave * 12.0f + oscASemitone + voicePitchMod + globalPitchOffset + fmMod;
                            float stringFrequency = voices[v].currentFrequency * std::pow(2.0f, pitchShift / 12.0f);
                            unisonSample = getNextSampleForString(voices[v].stringA, voices[v].level, stringFrequency, oscAStringMode, 16);
                        }
                        else if (oscAChoirMode > 0)
                        {
                            float pitchShift = oscAOctave * 12.0f + oscASemitone + voicePitchMod + globalPitchOffset + fmMod;
                            float fineCents  = (oscAFine + detuneOffset) / 100.0f;
                            float choirFrequency = voices[v].currentFrequency * std::pow(2.0f, (pitchShift + fineCents) / 12.0f);
                            float choirLevel = voices[v].level * (oscALevel / 100.0f);
                            unisonSample = getNextSampleForChoir(voices[v].choirA, choirLevel, choirFrequency, oscAChoirMode);
                        }
                        else
                        {
                            unisonSample = getNextSampleForVoice(voices[v], oscAWave,
                                                                  oscAOctave, oscASemitone + voicePitchMod + globalPitchOffset + fmMod,
                                                                  oscAFine + detuneOffset,
                                                                  oscALevel, oscAPhase, oscAWtPos);
                        }

                        sampleALeft  += unisonSample * oscAUnisonVoiceGains[u] * oscAUnisonLeftGains[u];
                        sampleARight += unisonSample * oscAUnisonVoiceGains[u] * oscAUnisonRightGains[u];
                    }
                }

                // Generate unison voices for OSC B
                float sampleBLeft = 0.0f;
                float sampleBRight = 0.0f;
                if (oscBPowerOn)
                {
                    for (int u = 0; u < oscBUnison; ++u)
                    {
                        float detuneOffset = (oscBUnison > 1)
                            ? oscBUnisonSpreadPos[u] * oscBDetune : 0.0f;

                        // Apply CHAOS - add random variation to each voice
                        if (oscBChaos > 0.1f && oscBUnison > 1)
                        {
                            float randomSeed = (float)(v * 1000 + u * 137 + sample % 100);
                            float randomValue = std::sin(randomSeed) * 0.5f + 0.5f;

                            float chaosAmount = oscBChaos / 100.0f;
                            float chaosPitch = (randomValue - 0.5f) * 2.0f * chaosAmount * 50.0f;
                            detuneOffset += chaosPitch;
                        }

                        float unisonSample;

                        // Synthesis priority: String > Choir > Wavetable
                        if (oscBStringMode > 0)
                        {
                            float pitchShift = oscBOctave * 12.0f + oscBSemitone + voicePitchMod + globalPitchOffset;
                            float stringFrequency = voices[v].currentFrequency * std::pow(2.0f, pitchShift / 12.0f);
                            unisonSample = getNextSampleForString(voices[v].stringB, voices[v].level, stringFrequency, oscBStringMode, 16);
                        }
                        else if (oscBChoirMode > 0)
                        {
                            float pitchShift = oscBOctave * 12.0f + oscBSemitone + voicePitchMod + globalPitchOffset;
                            float fineCents  = (oscBFine + detuneOffset) / 100.0f;
                            float choirFrequency = voices[v].currentFrequency * std::pow(2.0f, (pitchShift + fineCents) / 12.0f);
                            float choirLevel = voices[v].level * (oscBLevel / 100.0f);
                            unisonSample = getNextSampleForChoir(voices[v].choirB, choirLevel, choirFrequency, oscBChoirMode);
                        }
                        else
                        {
                            unisonSample = getNextSampleForVoice(voices[v], oscBWave,
                                                                  oscBOctave, oscBSemitone + voicePitchMod + globalPitchOffset,
                                                                  oscBFine + detuneOffset,
                                                                  oscBLevel, oscBPhase, oscBWtPos);
                        }

                        sampleBLeft  += unisonSample * oscBUnisonVoiceGains[u] * oscBUnisonLeftGains[u];
                        sampleBRight += unisonSample * oscBUnisonVoiceGains[u] * oscBUnisonRightGains[u];
                    }
                }

                // Mix the oscillators with pre-computed global panning
                float voiceLeft = (sampleALeft * leftGainA) + (sampleBLeft * leftGainB);
                float voiceRight = (sampleARight * rightGainA) + (sampleBRight * rightGainB);

                // Apply ENV 3 pan modulation if selected
                if (env3Dest == 2)
                {
                    float panAmount = juce::jlimit(-1.0f, 1.0f, voicePanMod);
                    // Linear pan law approximation (avoids sqrt per voice per sample)
                    float leftPanGain  = 0.5f * (1.0f - panAmount);
                    float rightPanGain = 0.5f * (1.0f + panAmount);
                    voiceLeft *= leftPanGain;
                    voiceRight *= rightPanGain;
                }

                // ----------------------------------------------------------
                // Sub Oscillator ÃƒÆ’Ã‚Â¢Ãƒâ€šÃ¢â€šÂ¬Ãƒâ€šÃ¢â‚¬Â runs one octave below the MIDI note
                // ----------------------------------------------------------
                float subLeft = 0.0f;
                float subRight = 0.0f;
                if (subPowerOn)
                {
                    float subPitchShift = subOctave * 12.0f + globalPitchOffset;
                    float subFrequency  = voices[v].currentFrequency * std::pow (2.0f,
                                         subPitchShift / 12.0f);
                    float subDelta      = (float)wavetableSize * subFrequency
                                         / (float)currentSampleRate;

                    float subSample = 0.0f;
                    float subPhase  = voices[v].subIndex / (float)wavetableSize;

                    if (subWave == 0) // Sine
                        subSample = std::sin (juce::MathConstants<float>::twoPi * subPhase);
                    else              // Square
                        subSample = (subPhase < 0.5f) ? 1.0f : -1.0f;

                    voices[v].subIndex += subDelta;
                    if (voices[v].subIndex >= (float)wavetableSize)
                        voices[v].subIndex -= (float)wavetableSize;

                    float subOut = subSample * voices[v].level * (subLevel / 100.0f);

                    subLeft  = subOut * subLeftGain;
                    subRight = subOut * subRightGain;
                }

                // ----------------------------------------------------------
                // Apply amplitude envelope to THIS voice only, then add to mix
                // ----------------------------------------------------------
                // HUMANIZE: subtle per-voice level shimmer
                float humanGain = 1.0f;
                if (modHuman > 0.0f) {
                    float humanPhase = std::sin((float)v * 1.3f + (float)sample * 0.0001f);
                    humanGain = 1.0f + humanPhase * (modHuman / 10.0f) * 0.08f;
                }

                // TIGHTEN: reduce amplitude in the attack portion of the envelope
                float tightenScale = 1.0f;
                if (modTighten > 0.0f && voices[v].ampEnvLevel < 0.3f) {
                    float tightenAmt = (modTighten / 10.0f) * 0.7f;
                    tightenScale = 1.0f - tightenAmt * (1.0f - voices[v].ampEnvLevel / 0.3f);
                    tightenScale = juce::jlimit(0.0f, 1.0f, tightenScale);
                }

                // FATTNESS: subtle opposing L/R phase shimmer for stereo width
                float fatLeft  = voiceLeft;
                float fatRight = voiceRight;
                if (modFattness > 0.0f) {
                    float fatAmt   = (modFattness / 10.0f) * 0.15f;
                    float fatPhase = std::sin((float)v * 0.7f + (float)sample * 0.00007f) * fatAmt;
                    fatLeft  *= (1.0f + fatPhase);
                    fatRight *= (1.0f - fatPhase);
                }

                float ampScale = voices[v].ampEnvLevel * humanGain * tightenScale;
                mixedSampleLeft  += (fatLeft  + subLeft)  * ampScale;
                mixedSampleRight += (fatRight + subRight) * ampScale;

                // Accumulate per-voice averages in-line (avoids 6 extra loops over all voices)
                ++currentActiveVoices;
                sumFilterEnvLevel += voices[v].filterEnvLevel;
                sumLfo0           += voices[v].lfoSmoothed[0];
                sumNoteTrack      += voices[v].noteTrackValue;
                sumEnv3           += voices[v].env3Level;
                for (int l = 0; l < 4; ++l)
                    sumLfo[l] += voices[v].lfoSmoothed[l];
                sumAmpEnv    += voices[v].ampEnvLevel;
                sumFilterEnv += voices[v].filterEnvLevel;
                sumEnv3Mat   += voices[v].env3Level;
                sumVel       += voices[v].shapedVelocity;
                sumNote      += voices[v].noteTrackValue;
            }
        }

        // ------------------------------------------------------------------
        // Polyphony gain compensation - prevent clipping when multiple voices play
        // ------------------------------------------------------------------
        if (currentActiveVoices > 1)
        {
            float polyGain = 1.0f / std::sqrt((float)currentActiveVoices);
            mixedSampleLeft  *= polyGain;
            mixedSampleRight *= polyGain;
        }

        // ------------------------------------------------------------------
        // Noise Generator ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â FREE mode runs always, KEY mode only when notes held
        // ------------------------------------------------------------------
        if (noisePowerOn && (!noiseKeyMode || anyVoiceActive))
        {
            // Raw white noise: uniform random -1 to +1
            float white = noiseRandom.nextFloat() * 2.0f - 1.0f;

            float noiseSample = 0.0f;

            if (noiseType == 0) // White ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â flat spectrum
            {
                noiseSample = white;
            }
            else if (noiseType == 1) // Pink ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â Paul Kellet 3-stage approx
            {
                pinkB0 = 0.99886f * pinkB0 + white * 0.0555179f;
                pinkB1 = 0.99332f * pinkB1 + white * 0.0750759f;
                pinkB2 = 0.96900f * pinkB2 + white * 0.1538520f;
                noiseSample = (pinkB0 + pinkB1 + pinkB2 + white * 0.5362f) * 0.2f;
            }
            else // Brown ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â leaky integrator (deeper rolloff than pink)
            {
                brownAccum = (brownAccum + (0.02f * white)) / 1.02f;
                noiseSample = brownAccum * 3.5f; // compensate for level drop
            }

            // Apply noise-specific LFO amount (uses LFO 1 average across active voices)
            float noiseLfoScale = 1.0f;
            if (noiseLfoAmt > 0.0f)
            {
                float lfo1Avg = (currentActiveVoices > 0) ? sumLfo0 / (float)currentActiveVoices : 0.0f;
                // lfo1Avg is -1..+1; scale so that at full amount it fully modulates the level
                noiseLfoScale = juce::jlimit(0.0f, 2.0f, 1.0f + lfo1Avg * (noiseLfoAmt / 100.0f));
            }

            // Apply simple 1-pole lowpass tone control to noise (noiseCutoffHz)
            float noiseOut = noiseSample * (noiseLevel / 100.0f) * noiseEnvGain * noiseLfoScale;

            // 1-pole lowpass: coefficient
            if (noiseCutoffHz < 19900.0f)
            {
                float nLP = 1.0f / (1.0f + (float)currentSampleRate
                            / (juce::MathConstants<float>::twoPi * noiseCutoffHz));
                float nOutL = noiseLPStateL + nLP * (noiseOut - noiseLPStateL);
                noiseLPStateL   = nOutL;
                float nOutR = noiseLPStateR + nLP * (noiseOut - noiseLPStateR);
                noiseLPStateR   = nOutR;
                mixedSampleLeft  += nOutL * noiseLeftGain;
                mixedSampleRight += nOutR * noiseRightGain;
            }
            else
            {
                mixedSampleLeft  += noiseOut * noiseLeftGain;
                mixedSampleRight += noiseOut * noiseRightGain;
            }
        }

        // ------------------------------------------------------------------
        // Use accumulators from the main voice loop (no extra loops needed)
        // ------------------------------------------------------------------
        const float invActive = (currentActiveVoices > 0)
                                ? 1.0f / (float)currentActiveVoices : 0.0f;

        float filterEnvMix = sumFilterEnvLevel * invActive;
        float envCutoff = baseCutoffHz + fEnvAmount * filterEnvMix * 18000.0f;

        float lfoMix = sumLfo0 * invActive;
        envCutoff += fLfoAmount * lfoMix * 10000.0f;

        float noteTrackMix = sumNoteTrack * invActive;
        envCutoff += noteTrackMix * 8000.0f;
        envCutoff = juce::jlimit (20.0f, 20000.0f, envCutoff);

        // FM Filter: OSC B phase modulates filter cutoff
        // We use the average note frequency across active voices to keep the
        // modulator harmonically locked to the played pitch, just like real FM.
        if (modFmFilter > 0.0f && currentActiveVoices > 0)
        {
            // Compute average played frequency across active voices
            float avgFreq = 0.0f;
            int activeCount = 0;
            for (int v = 0; v < maxVoices; ++v)
            {
                if (voices[v].isActive)
                {
                    avgFreq += voices[v].currentFrequency;
                    ++activeCount;
                }
            }
            if (activeCount > 0)
                avgFreq /= (float)activeCount;
            else
                avgFreq = 440.0f;

            // Advance the FM filter phase accumulator
            float fmFilterDelta = juce::MathConstants<float>::twoPi * avgFreq
                                  / (float)currentSampleRate;
            oscBFmAccum += fmFilterDelta;
            if (oscBFmAccum >= juce::MathConstants<float>::twoPi)
                oscBFmAccum -= juce::MathConstants<float>::twoPi;

            // Modulation depth: at 100% this sweeps ±4000 Hz around the cutoff.
            // Scaled softly so low amounts feel subtle and musical.
            float fmDepth = (modFmFilter / 100.0f) * (modFmFilter / 100.0f) * 4000.0f;
            float fmFilterMod = std::sin(oscBFmAccum) * fmDepth;
            envCutoff = juce::jlimit(20.0f, 20000.0f, envCutoff + fmFilterMod);
        }

        float env3Mix = sumEnv3 * invActive;
        
        float env3Modulation = env3Mix * env3Amount;
        
        // Apply ENV 3 based on selected destination
        // Destination: 0=Filter Res, 1=Pitch, 2=Pan, 3=Drive
        float modRes = baseRes;
        float modDrive = baseDrive;
        float pitchModSemitones = 0.0f;
        float panMod = 0.0f;
        
        if (env3Dest == 0) // Filter Resonance
        {
            modRes = juce::jlimit (0.0f, 1.0f, baseRes + env3Modulation);
        }
        else if (env3Dest == 1) // Pitch (ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â±12 semitones range)
        {
            pitchModSemitones = env3Modulation * 12.0f;  // ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â±12 semitones at full amount
        }
        else if (env3Dest == 2) // Pan (stereo width modulation)
        {
            panMod = env3Modulation;  // -1 to +1 range
        }
        else if (env3Dest == 3) // Drive
        {
            modDrive = juce::jlimit (1.0f, 10.0f, baseDrive + env3Modulation * 5.0f);
        }

        // Apply mod wheel to filter cutoff (dest == 0) or resonance (dest == 1)
        if (modWheelDest == 0)
            envCutoff = juce::jlimit(20.0f, 20000.0f, envCutoff + currentModWheel * 8000.0f);
        else if (modWheelDest == 1)
            modRes = juce::jlimit(0.0f, 1.0f, modRes + currentModWheel * 0.5f);

        // ------------------------------------------------------------------
        // MODULATION MATRIX — apply active slots
        // Averages already computed in-line from the main voice loop
        // ------------------------------------------------------------------
        {
            float avgLfo[4];
            for (int l = 0; l < 4; ++l) avgLfo[l] = sumLfo[l] * invActive;
            float avgEnv[3] = { sumAmpEnv * invActive, sumFilterEnv * invActive, sumEnv3Mat * invActive };
            float avgVel  = sumVel  * invActive;
            float avgNote = sumNote * invActive;

            for (int i = 0; i < 16; ++i)
            {
                if (!matrixSlots[i].enabled) continue;
                if (matrixSlots[i].source == 0 || matrixSlots[i].dest == 0) continue;

                // Resolve source signal (-1..+1 range)
                float sig = 0.0f;
                switch (matrixSlots[i].source)
                {
                    case 1:  sig = avgLfo[0]; break;            // LFO 1
                    case 2:  sig = avgLfo[1]; break;            // LFO 2
                    case 3:  sig = avgLfo[2]; break;            // LFO 3
                    case 4:  sig = avgLfo[3]; break;            // LFO 4
                    case 5:  sig = avgEnv[0]; break;            // ENV 1 (amp)
                    case 6:  sig = avgEnv[1]; break;            // ENV 2 (filter)
                    case 7:  sig = avgEnv[2]; break;            // ENV 3
                    case 8:  sig = avgVel * 2.0f - 1.0f; break; // Velocity 0-1 → -1..+1
                    case 9:  sig = avgNote; break;              // Note track -1..+1
                    case 10: sig = currentModWheel * 2.0f - 1.0f; break; // Mod Wheel
                    case 11: sig = currentPitchBend; break;    // Pitch Bend -1..+1
                    default: break;
                }

                float mod = sig * matrixSlots[i].amount;

                // Apply to destination
                switch (matrixSlots[i].dest)
                {
                    // OSC A Pitch (±12 semitones max)
                    case 1:  /* handled per-voice — stored as global offset for now */
                             pitchModSemitones += mod * 12.0f; break;
                    // OSC A Level — no per-sample way here; skip (per-voice only practical)
                    case 2:  break;
                    // OSC A Pan — skip (per-voice)
                    case 3:  break;
                    // OSC A WT Pos — skip (per-voice, handled in getNextSampleForVoice)
                    case 4:  break;
                    // OSC A Detune — skip (per-voice)
                    case 5:  break;
                    // OSC B Pitch
                    case 6:  pitchModSemitones += mod * 12.0f; break;
                    // OSC B Level/Pan/WtPos/Detune — skip (per-voice)
                    case 7:  break;
                    case 8:  break;
                    case 9:  break;
                    case 10: break;
                    // Filter Cutoff (mod maps ±10kHz)
                    case 11: envCutoff = juce::jlimit(20.0f, 20000.0f, envCutoff + mod * 10000.0f); break;
                    // Filter Resonance (mod maps ±0.9)
                    case 12: modRes = juce::jlimit(0.0f, 1.0f, modRes + mod * 0.9f); break;
                    // Filter Drive (mod maps ±4.5)
                    case 13: modDrive = juce::jlimit(1.0f, 10.0f, modDrive + mod * 4.5f); break;
                    // ENV 1 Attack/Decay/Sustain/Release — runtime modulation not practical per-sample; skip
                    case 14: break;
                    case 15: break;
                    case 16: break;
                    case 17: break;
                    // LFO Rate modulation — skip (requires per-block restructure)
                    case 18: break;
                    case 19: break;
                    case 20: break;
                    case 21: break;
                    // Master Volume (mod maps ±50%)
                    case 22: mixedSampleLeft  *= (1.0f + mod * 0.5f);
                             mixedSampleRight *= (1.0f + mod * 0.5f); break;
                    // Master Pan (mod maps full ±1)
                    case 23: panMod += mod; break;
                    default: break;
                }
            }
        }

        filterLeft.setParams  (envCutoff, modRes, modDrive);
        filterRight.setParams (envCutoff, modRes, modDrive);

        // Apply Moog Ladder Filter to the mixed stereo output
        // ------------------------------------------------------------------
        mixedSampleLeft  = filterLeft.processSample  (mixedSampleLeft);
        mixedSampleRight = filterRight.processSample (mixedSampleRight);

        // HP FILTER (1-pole highpass) — only runs if modHpFilter > 0
        if (modHpFilter > 0.0f) {
            float hpOutL = hpAlpha * (hpPrevOutL + mixedSampleLeft - hpPrevInL);
            hpPrevInL    = mixedSampleLeft;
            hpPrevOutL   = hpOutL;
            mixedSampleLeft = hpOutL;

            float hpOutR = hpAlpha * (hpPrevOutR + mixedSampleRight - hpPrevInR);
            hpPrevInR    = mixedSampleRight;
            hpPrevOutR   = hpOutR;
            mixedSampleRight = hpOutR;
        }

        // ------------------------------------------------------------------
        // Apply MASTER section: Volume, Pan, (Tune and Transpose are applied
        // per-voice via pitchShift below — see bendAndTuneSemitones)
        // ------------------------------------------------------------------
        float masterPan = juce::jlimit(-100.0f, 100.0f, masterPanBase + panMod * 100.0f);
        float panLeft   = juce::jlimit(0.0f, 1.0f, 1.0f - masterPan);
        float panRight  = juce::jlimit(0.0f, 1.0f, 1.0f + masterPan);
        mixedSampleLeft  *= masterVol * panLeft;
        mixedSampleRight *= masterVol * panRight;

        // Apply mod wheel to volume if destination == 2
        // modWheelDest is hoisted to the top of the sample loop
        if (modWheelDest == 2)
        {
            float modVolScale = 1.0f - currentModWheel * 0.5f; // attenuates up to 50%
            mixedSampleLeft  *= modVolScale;
            mixedSampleRight *= modVolScale;
        }

        // Write to output channels (stereo)
        if (totalNumOutputChannels >= 2)
        {
            buffer.getWritePointer (0)[sample] = mixedSampleLeft;
            buffer.getWritePointer (1)[sample] = mixedSampleRight;
        }
        else if (totalNumOutputChannels == 1)
        {
            // Mono output - mix left and right
            buffer.getWritePointer (0)[sample] = (mixedSampleLeft + mixedSampleRight) * 0.5f;
        }
    } // end sample loop

    // MIDI Thru: restore saved messages so the DAW passes them downstream
    if (midiThruOn == 1)
        midiMessages = thruBuffer;
    else
        midiMessages.clear();

    // =========================================================================
    // FX RACK CHAIN — process audio through active rack plugins
    // =========================================================================
    {
        juce::SpinLock::ScopedLockType lock (fxChainLock);
        juce::MidiBuffer emptyMidi;
        const int slots = numFxSlots.load();
        for (int i = 0; i < slots; ++i)
        {
            auto& slot = fxChain[i];
            if (slot.processor != nullptr && slot.powered && ! slot.muted)
                slot.processor->processBlock (buffer, emptyMidi);
        }
    }

} // end processBlock

//==============================================================================
bool ARKAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ARKAudioProcessor::createEditor()
{
    return new ARKAudioProcessorEditor (*this);
}

//==============================================================================
void ARKAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Save extra non-APVTS state into the tree
    state.setProperty("currentFullPresetName",   currentFullPresetName,   nullptr);
    state.setProperty("currentFullPresetFolder", currentFullPresetFolder, nullptr);
    state.setProperty("currentOscAPresetName",   currentOscAPresetName,   nullptr);
    state.setProperty("currentOscBPresetName",   currentOscBPresetName,   nullptr);
    state.setProperty("currentOscAPresetIndex",  currentOscAPresetIndex,  nullptr);
    state.setProperty("currentOscBPresetIndex",  currentOscBPresetIndex,  nullptr);
    state.setProperty("keyboardOctaveOffset",    (int)keyboardOctaveOffset.load(), nullptr);
    state.setProperty("keyboardSemiOffset",      (int)keyboardSemiOffset.load(),   nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    // Capture FX rack state (from live FXPage if editor is open, otherwise from cache)
    if (fxPage != nullptr)
    {
        auto fxStateXml = fxPage->saveFXState();
        if (fxStateXml != nullptr)
        {
            cachedFXState = std::make_unique<juce::XmlElement> (*fxStateXml);
            xml->addChildElement (fxStateXml.release());
        }
    }
    else if (cachedFXState != nullptr)
    {
        xml->addChildElement (new juce::XmlElement (*cachedFXState));
    }

    copyXmlToBinary (*xml, destData);
}

void ARKAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState == nullptr)
        return;

    auto newState = juce::ValueTree::fromXml (*xmlState);
    if (!newState.hasType (apvts.state.getType()))
        return;

    // Restore extra non-APVTS state before replacing (properties are on the tree)
    currentFullPresetName   = newState.getProperty("currentFullPresetName",   "Init Preset").toString();
    currentFullPresetFolder = newState.getProperty("currentFullPresetFolder", "User Presets").toString();
    currentOscAPresetName   = newState.getProperty("currentOscAPresetName",   "---").toString();
    currentOscBPresetName   = newState.getProperty("currentOscBPresetName",   "---").toString();
    currentOscAPresetIndex  = (int)newState.getProperty("currentOscAPresetIndex", -1);
    currentOscBPresetIndex  = (int)newState.getProperty("currentOscBPresetIndex", -1);
    keyboardOctaveOffset.store((int)newState.getProperty("keyboardOctaveOffset", 0));
    keyboardSemiOffset.store  ((int)newState.getProperty("keyboardSemiOffset",   0));

    apvts.replaceState (newState);

    // Cache and restore FX rack state
    auto* fxStateXml = xmlState->getChildByName ("FXSTATE");
    if (fxStateXml != nullptr)
    {
        cachedFXState = std::make_unique<juce::XmlElement> (*fxStateXml);

        if (fxPage != nullptr)
            fxPage->restoreFXState (fxStateXml);
    }
}

//==============================================================================
// OSC Preset Management
//==============================================================================

juce::File ARKAudioProcessor::getOscPresetFolder() const
{
    auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("ARK")
                    .getChildFile ("OSC Presets");
    if (! folder.exists())
        folder.createDirectory();
    return folder;
}

juce::File ARKAudioProcessor::getFullPresetRootFolder() const
{
    // Root:  <Documents>/ARK/Presets/
    auto root = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("ARK")
                    .getChildFile("Presets");
    if (!root.exists())
        root.createDirectory();
    // Always ensure the default "User Presets" folder exists
    auto userFolder = root.getChildFile("User Presets");
    if (!userFolder.exists())
        userFolder.createDirectory();
    return root;
}

std::unique_ptr<juce::XmlElement> ARKAudioProcessor::buildOscPresetXml (int oscIndex) const
{
    // oscIndex: 0 = OSC A, 1 = OSC B
    const juce::String prefix = (oscIndex == 0) ? "oscA" : "oscB";
    auto xml = std::make_unique<juce::XmlElement> ("ARKOscPreset");
    xml->setAttribute ("version", 1);
    xml->setAttribute ("osc", prefix);

    // List of every parameter belonging to this oscillator
    juce::StringArray paramIds = {
        prefix + "Wave",    prefix + "Octave",   prefix + "Semitone",
        prefix + "Fine",    prefix + "Unison",   prefix + "Detune",
        prefix + "Blend",   prefix + "WtPos",    prefix + "Pan",
        prefix + "Level",   prefix + "Phase",    prefix + "Spread",
        prefix + "Chaos",   prefix + "StringMode", prefix + "ChoirMode"
    };

    for (const auto& id : paramIds)
    {
        auto* param = apvts.getRawParameterValue (id);
        if (param != nullptr)
            xml->setAttribute (id, (double)param->load());
    }

    return xml;
}

void ARKAudioProcessor::applyOscPresetXml (int oscIndex, const juce::XmlElement& xml)
{
    const juce::String prefix = (oscIndex == 0) ? "oscA" : "oscB";

    juce::StringArray paramIds = {
        prefix + "Wave",    prefix + "Octave",   prefix + "Semitone",
        prefix + "Fine",    prefix + "Unison",   prefix + "Detune",
        prefix + "Blend",   prefix + "WtPos",    prefix + "Pan",
        prefix + "Level",   prefix + "Phase",    prefix + "Spread",
        prefix + "Chaos",   prefix + "StringMode", prefix + "ChoirMode"
    };

    for (const auto& id : paramIds)
    {
        if (xml.hasAttribute (id))
        {
            auto* param = apvts.getParameter (id);
            if (param != nullptr)
            {
                float val = (float)xml.getDoubleAttribute (id);
                auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param);
                if (rangedParam != nullptr)
                {
                    float norm = rangedParam->convertTo0to1 (val);
                    param->setValueNotifyingHost (norm);
                }
            }
        }
    }
}

void ARKAudioProcessor::refreshOscPresetList()
{
    oscPresetList.clear();
    auto folder = getOscPresetFolder();
    auto files = folder.findChildFiles (juce::File::findFiles, false, "*.xml");
    files.sort();
    for (const auto& f : files)
        oscPresetList.add (f.getFileNameWithoutExtension());
}

void ARKAudioProcessor::saveOscPreset (int oscIndex, const juce::String& presetName)
{
    auto folder = getOscPresetFolder();
    auto file   = folder.getChildFile (presetName + ".xml");

    auto xml = buildOscPresetXml (oscIndex);
    xml->writeTo (file);

    refreshOscPresetList();

    int newIndex = oscPresetList.indexOf (presetName);
    if (oscIndex == 0)
    {
        currentOscAPresetName  = presetName;
        currentOscAPresetIndex = newIndex;
    }
    else
    {
        currentOscBPresetName  = presetName;
        currentOscBPresetIndex = newIndex;
    }
}

void ARKAudioProcessor::loadOscPreset (int oscIndex, int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= oscPresetList.size())
        return;

    auto folder = getOscPresetFolder();
    auto file   = folder.getChildFile (oscPresetList[presetIndex] + ".xml");

    if (! file.existsAsFile())
        return;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
        return;

    applyOscPresetXml (oscIndex, *xml);

    if (oscIndex == 0)
    {
        currentOscAPresetName  = oscPresetList[presetIndex];
        currentOscAPresetIndex = presetIndex;
    }
    else
    {
        currentOscBPresetName  = oscPresetList[presetIndex];
        currentOscBPresetIndex = presetIndex;
    }
}

int ARKAudioProcessor::stepOscPreset (int oscIndex, int delta)
{
    if (oscPresetList.isEmpty())
        return -1;

    int current = (oscIndex == 0) ? currentOscAPresetIndex : currentOscBPresetIndex;
    int next = (current + delta + oscPresetList.size()) % oscPresetList.size();
    loadOscPreset (oscIndex, next);
    return next;
}

//==========================================================================
// Full Synth Preset Management
//==========================================================================

void ARKAudioProcessor::saveFullPreset(const juce::String& presetName,
                                       const juce::String& folderName)
{
    // Resolve / create the target folder
    auto folder = getFullPresetRootFolder().getChildFile(folderName);
    if (!folder.exists())
        folder.createDirectory();

    // Serialise the entire APVTS state to XML
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml == nullptr)
        return;

    // Write to disk
    auto file = folder.getChildFile(presetName + ".xml");
    xml->writeTo(file);

    // Update internal state
    currentFullPresetName   = presetName;
    currentFullPresetFolder = folderName;
}

bool ARKAudioProcessor::loadFullPreset(const juce::String& presetName,
                                       const juce::String& folderName)
{
    auto file = getFullPresetRootFolder()
                    .getChildFile(folderName)
                    .getChildFile(presetName + ".xml");

    if (!file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return false;

    // Restore APVTS state
    if (!xml->hasTagName(apvts.state.getType()))
        return false;

    apvts.replaceState(juce::ValueTree::fromXml(*xml));

    currentFullPresetName   = presetName;
    currentFullPresetFolder = folderName;
    return true;
}

bool ARKAudioProcessor::createPresetFolder(const juce::String& folderName)
{
    if (folderName.trim().isEmpty())
        return false;

    auto folder = getFullPresetRootFolder().getChildFile(folderName);
    if (folder.exists())
        return true;   // already exists — not an error

    return folder.createDirectory();
}

juce::StringArray ARKAudioProcessor::getPresetFolders() const
{
    auto root = getFullPresetRootFolder();

    // Stock preset pack folders — always present, in this fixed order
    const juce::StringArray stockFolders = {
        "Basic", "Ethereal", "Circuit", "Analog Soul",
        "Dark Matter", "Modular", "Pure", "Yamaha DX7 Library"
    };

    // Ensure stock folders exist on disk
    for (auto& name : stockFolders)
    {
        auto dir = root.getChildFile(name);
        if (!dir.exists())
            dir.createDirectory();
    }

    // Scan disk for any additional user-created folders
    juce::StringArray extras;
    for (auto& entry : juce::RangedDirectoryIterator(root, false,
                                                      "*",
                                                      juce::File::findDirectories))
    {
        juce::String dirName = entry.getFile().getFileName();
        if (!stockFolders.contains(dirName) && dirName != "User Presets")
            extras.add(dirName);
    }
    extras.sortNatural();

    // Build final list: stock folders first, then User Presets, then extras
    juce::StringArray folders;
    for (auto& name : stockFolders)
        folders.add(name);
    folders.add("User Presets");
    for (auto& name : extras)
        folders.add(name);

    return folders;
}

juce::StringArray ARKAudioProcessor::getPresetsInFolder(
                      const juce::String& folderName) const
{
    auto folder = getFullPresetRootFolder().getChildFile(folderName);
    juce::StringArray names;

    if (!folder.isDirectory())
        return names;

    for (auto& entry : juce::RangedDirectoryIterator(folder, false,
                                                      "*.xml",
                                                      juce::File::findFiles))
    {
        names.add(entry.getFile().getFileNameWithoutExtension());
    }

    names.sortNatural();
    return names;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ARKAudioProcessor();
}
