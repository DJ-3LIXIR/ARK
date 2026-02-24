/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin processor.
  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>

//==============================================================================
// Voice structure - MUST be defined before the class that uses it
struct Voice
{
    int midiNote = -1;
    float currentIndex = 0.0f;
    float subIndex     = 0.0f;
    float level = 0.0f;
    bool isActive = false;

    // ---- Amplitude Envelope (ENV 1) ----
    enum class EnvStage { Idle, Attack, Decay, Sustain, Release };
    EnvStage ampStage    = EnvStage::Idle;
    float    ampEnvLevel = 0.0f;   // current envelope output 0-1

    // ---- Filter Envelope (ENV 2) ----
    EnvStage filterStage    = EnvStage::Idle;
    float    filterEnvLevel = 0.0f; // current filter envelope output 0-1

    // ---- Envelope 3 (Assignable Modulation) ----
    EnvStage env3Stage    = EnvStage::Idle;
    float    env3Level = 0.0f; // current env3 envelope output 0-1

    // ---- LFO States (4 LFOs per voice) ----
    float lfoPhase[4]      = {0.f, 0.f, 0.f, 0.f};
    float lfoValue[4]      = {0.f, 0.f, 0.f, 0.f};
    float lfoDelayTime[4]  = {0.f, 0.f, 0.f, 0.f};
    float lfoRiseLevel[4]  = {0.f, 0.f, 0.f, 0.f};
    bool  lfoActive[4]     = {false, false, false, false};
    float lfoSmoothed[4]   = {0.f, 0.f, 0.f, 0.f};

    // ---- Portamento / Glide State ----
    float targetFrequency     = 440.0f;
    float currentFrequency    = 440.0f;
    float portaStartFrequency = 440.0f;
    int   lastMidiNote        = -1;
    float portaProgress       = 1.0f;

    // ---- Velocity & Note Tracking ----
    float shapedVelocity  = 0.0f;
    float noteTrackValue  = 0.0f;

    // ---- Voice Age (for stealing) ----
    unsigned long long voiceStartTime = 0;
};

//==============================================================================
// Moog Transistor Ladder Filter
struct MoogLadderFilter
{
    float stage[4]  = { 0.f, 0.f, 0.f, 0.f };
    float delay[4]  = { 0.f, 0.f, 0.f, 0.f };

    float sampleRate = 44100.f;
    float cutoff     = 20000.f;
    float resonance  = 0.f;
    float drive      = 1.f;

    int mode          = 0;  // 0=LP  1=HP  2=BP  3=Notch
    int characterMode = 0;  // 0=DEFAULT … 14=AMBER

    void reset()
    {
        for (int i = 0; i < 4; ++i) { stage[i] = 0.f; delay[i] = 0.f; }
    }

    void prepare (double sr) { sampleRate = (float)sr; reset(); }

    void setParams (float cutoffHz, float res, float driveAmt)
    {
        cutoff    = juce::jlimit (20.f, 20000.f, cutoffHz);
        resonance = juce::jlimit (0.f,  1.0f,    res);
        drive     = juce::jlimit (1.f,  10.f,    driveAmt);
    }

    static inline float softClip (float x) noexcept { return tanhf (x); }
    static inline float hardClip (float x) noexcept { return juce::jlimit (-1.0f, 1.0f, x); }
    static inline float asymClip (float x) noexcept { return x > 0.f ? tanhf (x) : tanhf (x * 0.5f) * 1.4f; }
    static inline float sineFold (float x) noexcept { return sinf  (x * 1.5708f); }
    static inline float cubeSat  (float x) noexcept { float c = juce::jlimit(-1.f,1.f,x); return c - (c*c*c) * 0.333f; }
    static inline float noSat    (float x) noexcept { return x; }

    float processSample (float input) noexcept
    {
        float fbScale      = 1.0f;
        float fbCompensate = 0.15f;
        float outputGain   = 1.0f;

        float (*inputShaper)(float) = softClip;
        float (*stageShaper)(float) = softClip;

        switch (characterMode)
        {
            case 0:  inputShaper = softClip;  stageShaper = softClip;  fbScale = 1.0f;  fbCompensate = 0.15f; outputGain = 1.0f;   break;
            case 1:  inputShaper = noSat;     stageShaper = noSat;     fbScale = 0.85f; fbCompensate = 0.08f; outputGain = 1.0f;   break;
            case 2:  inputShaper = asymClip;  stageShaper = softClip;  fbScale = 1.05f; fbCompensate = 0.18f; outputGain = 0.95f;  break;
            case 3:  inputShaper = softClip;  stageShaper = cubeSat;   fbScale = 1.1f;  fbCompensate = 0.22f; outputGain = 0.9f;   break;
            case 4:  inputShaper = hardClip;  stageShaper = softClip;  fbScale = 1.15f; fbCompensate = 0.12f; outputGain = 0.85f;  break;
            case 5:  inputShaper = noSat;     stageShaper = softClip;  fbScale = 1.2f;  fbCompensate = 0.05f; outputGain = 1.05f;  break;
            case 6:  inputShaper = cubeSat;   stageShaper = softClip;  fbScale = 1.0f;  fbCompensate = 0.14f; outputGain = 1.0f;   break;
            case 7:  inputShaper = sineFold;  stageShaper = softClip;  fbScale = 1.1f;  fbCompensate = 0.10f; outputGain = 0.8f;   break;
            case 8:  inputShaper = sineFold;  stageShaper = hardClip;  fbScale = 1.2f;  fbCompensate = 0.08f; outputGain = 0.75f;  break;
            case 9:  inputShaper = asymClip;  stageShaper = asymClip;  fbScale = 0.95f; fbCompensate = 0.20f; outputGain = 0.9f;   break;
            case 10: inputShaper = noSat;     stageShaper = cubeSat;   fbScale = 0.9f;  fbCompensate = 0.06f; outputGain = 1.1f;   break;
            case 11: inputShaper = softClip;  stageShaper = softClip;  fbScale = 0.7f;  fbCompensate = 0.25f; outputGain = 1.0f;   break;
            case 12: inputShaper = cubeSat;   stageShaper = cubeSat;   fbScale = 1.05f; fbCompensate = 0.28f; outputGain = 0.85f;  break;
            case 13: inputShaper = softClip;  stageShaper = asymClip;  fbScale = 0.75f; fbCompensate = 0.30f; outputGain = 0.95f;  break;
            case 14: inputShaper = asymClip;  stageShaper = sineFold;  fbScale = 1.0f;  fbCompensate = 0.16f; outputGain = 0.88f;  break;
            default: inputShaper = softClip;  stageShaper = softClip;  break;
        }

        const float f  = (cutoff / sampleRate) * 1.16f;
        const float f2 = f * f;
        const float fb = resonance * fbScale * (1.0f - fbCompensate * f2);

        float x = input * drive;
        x -= delay[3] * fb * 4.0f;
        x = inputShaper (x * 0.5f) * 2.0f;

        stage[0] = x        * f + delay[0] * (1.0f - f);
        stage[1] = stage[0] * f + delay[1] * (1.0f - f);
        stage[2] = stage[1] * f + delay[2] * (1.0f - f);
        stage[3] = stage[2] * f + delay[3] * (1.0f - f);

        for (int i = 0; i < 4; ++i)
        {
            delay[i] = stage[i];
            stage[i] = stageShaper (stage[i]);
        }

        float out = 0.f;
        switch (mode)
        {
            case 0:  out = stage[3]; break;
            case 1:  out = x - stage[0] - stage[1] - stage[2] - stage[3]; break;
            case 2:  out = (stage[1] - stage[3]) * 2.0f; break;
            case 3:  out = x - (stage[1] - stage[3]) * 2.0f; break;
            default: out = stage[3]; break;
        }

        return out * outputGain;
    }
};

//==============================================================================
class ARKAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ARKAudioProcessor();
    ~ARKAudioProcessor() override;
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Public keyboard state - so editor can access it
    juce::MidiKeyboardState keyboardState;

    // APVTS - public so the Editor can create attachments
    juce::AudioProcessorValueTreeState apvts;

    // Helper to create the APVTS parameter layout
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // =========================================================================
    // OSC Preset Management (public API for the editor)
    // =========================================================================
    void saveOscPreset  (int oscIndex, const juce::String& presetName);
    void loadOscPreset  (int oscIndex, int presetIndex);
    int  stepOscPreset  (int oscIndex, int delta);
    void refreshOscPresetList();

    juce::StringArray getOscPresetList()    const { return oscPresetList; }
    juce::String      getOscAPresetName()   const { return currentOscAPresetName; }
    juce::String      getOscBPresetName()   const { return currentOscBPresetName; }
    int               getOscAPresetIndex()  const { return currentOscAPresetIndex; }
    int               getOscBPresetIndex()  const { return currentOscBPresetIndex; }

    // =========================================================================
    // Full Synth Preset Management  (header SAVE button + Preset Browser)
    //
    // Presets are saved as XML files at:
    //   <Documents>/ARK/Presets/<folderName>/<presetName>.xml
    //
    // "User Presets" is the default folder and is created automatically.
    // Additional folders can be created by the user via createPresetFolder().
    // =========================================================================

    // Save the complete APVTS state to disk.
    // folderName defaults to "User Presets"; the folder is created if needed.
    void saveFullPreset (const juce::String& presetName,
                         const juce::String& folderName = "User Presets");

    // Load a full preset by name from the given folder.
    // Returns true on success, false if the file could not be found/parsed.
    bool loadFullPreset (const juce::String& presetName,
                         const juce::String& folderName);

    // Create a new user-defined sub-folder inside the preset root.
    // Returns true if created successfully (or already exists).
    bool createPresetFolder (const juce::String& folderName);

    // Returns a list of all sub-folder names inside the preset root.
    // "User Presets" will always appear first.
    juce::StringArray getPresetFolders() const;

    // Returns all preset names (no extension) inside the given folder.
    juce::StringArray getPresetsInFolder (const juce::String& folderName) const;

    // Accessors so the editor can update the header label after load/save.
    juce::String getCurrentPresetName()   const { return currentFullPresetName; }
    juce::String getCurrentPresetFolder() const { return currentFullPresetFolder; }

    // =========================================================================
    // Piano controls state — written by editor, read by processBlock
    // =========================================================================
    std::atomic<int> incomingPitchBend    { 8192 };
    std::atomic<int> incomingModWheel     { 0 };
    std::atomic<int> keyboardOctaveOffset { 0 };
    std::atomic<int> keyboardSemiOffset   { 0 };

private:
    //==============================================================================
    static const int maxVoices = 32;
    Voice voices[maxVoices];

    static const int wavetableSize = 1024;
    static const int numWaveforms  = 4;
    juce::AudioSampleBuffer wavetables[numWaveforms];
    double currentSampleRate = 44100.0;

    void createWavetables();
    float getNextSampleForVoice (Voice& voice, int wavetableIndex,
                                 float octave, float semitone, float fine,
                                 float level, float phase, float wtPos) noexcept;
    Voice* findFreeVoice();
    Voice* findVoicePlayingNote (int midiNote);

    // -------------------------------------------------------------------------
    // LFO Helper Functions
    // -------------------------------------------------------------------------
    float calculateLFOWaveform (float phase, int waveType) noexcept;
    void  updateLFO (Voice& voice, int lfoIndex, float rate, float rise,
                     float delay, float smooth, int waveType, int trigMode) noexcept;

    // -------------------------------------------------------------------------
    // Moog Ladder Filter
    // -------------------------------------------------------------------------
    MoogLadderFilter filterLeft;
    MoogLadderFilter filterRight;

    // -------------------------------------------------------------------------
    // MOD Section — HP Filter state (1-pole highpass per channel)
    // -------------------------------------------------------------------------
    float hpPrevInL  = 0.0f;
    float hpPrevOutL = 0.0f;
    float hpPrevInR  = 0.0f;
    float hpPrevOutR = 0.0f;

    // -------------------------------------------------------------------------
    // Noise Generator
    // -------------------------------------------------------------------------
    juce::Random noiseRandom;
    float pinkB0 = 0.f, pinkB1 = 0.f, pinkB2 = 0.f;
    float brownAccum = 0.f;
    float noiseLPStateL = 0.0f;
    float noiseLPStateR = 0.0f;

    // -------------------------------------------------------------------------
    // MIDI Real-Time State
    // -------------------------------------------------------------------------
    float currentPitchBend = 0.0f;
    float currentModWheel  = 0.0f;

    // -------------------------------------------------------------------------
    // Voicing / Controls State
    // -------------------------------------------------------------------------
    unsigned long long voiceCounter = 0;
    int  heldNotes[128];
    int  heldNoteCount = 0;

    void pushHeldNote (int midiNote);
    void removeHeldNote (int midiNote);
    int  topHeldNote() const;
    Voice* findOldestVoice();
    Voice* findQuietestVoice();
    Voice* allocateVoice (int maxAllowed);
    float applyPortaCurve (float progress, float curveParam) const;
    float shapeVelocity (float rawVelocity, float curveParam) const;
    float calculateNoteTracking (int midiNote, float curveParam) const;

    // -------------------------------------------------------------------------
    // ARP/SEQ Engine State
    // -------------------------------------------------------------------------
    int   arpHeldNotes[128];
    int   arpHeldNoteCount = 0;
    int   arpSequence[512];
    int   arpSequenceLength = 0;
    int   arpCurrentStep = 0;
    int   arpCurrentNote = -1;
    int   arpSamplesUntilNext = 0;
    int   arpSamplesUntilGateOff = 0;
    bool  arpNoteIsOn  = false;
    bool  arpGoingUp   = true;
    float arpLastVelocity = 0.8f;
    juce::Random arpRandom;

    void arpPushNote (int midiNote);
    void arpRemoveNote (int midiNote);
    void arpBuildSequence (int pattern, int octaveRange);
    void arpNoteOn  (int midiNote, float velocity);
    void arpNoteOff (int midiNote);
    void arpReset();

    // -------------------------------------------------------------------------
    // OSC Preset State (private)
    // -------------------------------------------------------------------------
    juce::StringArray oscPresetList;
    juce::String      currentOscAPresetName  = "---";
    juce::String      currentOscBPresetName  = "---";
    int               currentOscAPresetIndex = -1;
    int               currentOscBPresetIndex = -1;

    juce::File getOscPresetFolder() const;
    void       applyOscPresetXml (int oscIndex, const juce::XmlElement& xml);
    std::unique_ptr<juce::XmlElement> buildOscPresetXml (int oscIndex) const;

    // -------------------------------------------------------------------------
    // Full Preset State (private)
    // -------------------------------------------------------------------------
    juce::String currentFullPresetName   = "Init Preset";
    juce::String currentFullPresetFolder = "User Presets";

    // Returns <Documents>/ARK/Presets/ — creates the directory tree on first call
    // including the default "User Presets" sub-folder.
    juce::File getFullPresetRootFolder() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ARKAudioProcessor)
};
