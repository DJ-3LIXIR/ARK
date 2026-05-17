ARK Synthesizer
A professional software synthesizer plugin by 3LIXIR MUSIC
ARK is a full-featured JUCE-based software synthesizer plugin built for professional music
production. It combines a deep dual-oscillator synthesis engine, a comprehensive modulation
system, and a built-in FX rack that hosts 3LIXIR MUSIC's own audio plugins as internal
processors — all in a single instrument.
Available as AU, VST3, and Standalone for macOS and Windows.
Features
Dual Oscillator Engine
OSC A & OSC B — independently configurable oscillators with waveform selection,
unison, detune, pan, and level controls
STR Mode — string synthesis models including Pluck, Strum, Pizzicato, and Arco
CHR Mode — choir sample engine with OOH, AAH, Women, and Men voicings using
bundled sample libraries
Sub Oscillator — dedicated sub layer for low-end reinforcement
Noise Generator — white/colored noise source for texture and sound design
Filter & Amp Section
Multi-mode filter with cutoﬀ, resonance, and envelope routing
Dedicated amplitude envelope with full ADSR control
Signal flow: OSC → Filter → Amp → FX Rack
Deep Modulation System
3 Envelopes — Amp, Filter, and a freely assignable modulation envelope
4 LFOs — each with multiple waveforms and trigger mode options
Modulation Matrix — route any source (velocity, aftertouch, mod wheel, LFOs, envelopes)
to any destination
Portamento — smooth pitch glide with mono/poly/legato voice modes
Arpeggiator — built-in arp with rate, range, and pattern control
Built-in FX Rack
ARK includes an internal FX rack architecture that loads 3LIXIR MUSIC plugins as per-slot
processors directly inside the instrument:
HADES — shuttle and waveform simulator
APOLLO — reverb and room simulator
ORION SOUND EQ — parametric equalizer
Each FX slot supports independent power, mute, and solo controls. Multiple slots can be chained
for full signal processing without leaving the instrument.
Preset System
Full synth preset save and load
Per-oscillator preset browser
Folder-based preset organization in the UI
Plugin Formats
Format Status
AU (Audio Unit) macOS
VST3 macOS / Windows
Standalone macOS / Windows
Manufacturer: 3LIXIR MUSIC
Product Name: ARK
Tech Stack
C++ — core synthesis and DSP
JUCE Framework — plugin architecture, UI, and audio processing
Xcode — macOS build (AU + VST3)
Visual Studio — Windows build (VST3)
Key Source Files
File Description
ARK.jucer JUCE project configuration
PluginProcessor.cpp/h Core audio engine, signal chain, voice management
PluginEditor.cpp/h Main UI and layout
FXPage.cpp/h Internal FX rack host architecture
Signal Flow
OSC A + OSC B (+ Sub + Noise)
↓
Filter Section
↓
Amp Envelope
↓
FX Rack (Hades → Apollo → Orion)
↓
Output
About
ARK is developed and maintained by Jared Frazier under the 3LIXIR MUSIC brand.
Website: 3lixirmusic.com
Spotify: 3LIXIR MUSIC on Spotify
YouTube: 3LIXIR MUSIC on YouTube
