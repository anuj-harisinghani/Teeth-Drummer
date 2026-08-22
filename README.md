# Teeth Drummer 🦷🥁
**Real-Time Oral Percussion Audio-to-MIDI Trigger & Drum Synthesizer (VST3 / Standalone)**

Teeth Drummer is a real-time, ultra-low latency audio plugin and standalone application that captures microphone audio of human teeth percussion (clicks, snaps, taps, and oral cavity acoustic resonances), categorizes each hit into drum elements (Kick, Snare, Closed Hi-Hat, Open Hi-Hat), and outputs sample-accurate MIDI events to trigger drum samplers in any DAW.

---

## Key Features

- ⚡ **Ultra-Low Latency DSP Engine**: Sub-2ms micro-transient detection combined with 3-band energy decomposition (Low: 60–350Hz, Mid: 350–3500Hz, High: 3500–12000Hz) and zero-crossing analysis on the initial 2.9ms transient burst.
- 🎯 **Intelligent Hybrid Classification**:
  - **Out-of-the-box Factory Heuristics**: Immediate plug-and-play operation.
  - **Interactive Calibration Wizard ("Learn Pad")**: Tap your teeth 5 times to adapt to unique dental geometries and mouth cavity formants.
- 🎹 **Sample-Accurate MIDI Generation**:
  - Configurable MIDI note mappings (General MIDI standard default: Kick=36, Snare=38, Closed Hat=42, Open Hat=46).
  - Dynamic velocity curve translation (Peak dB SPL $\rightarrow$ MIDI 1–127).
- 🔊 **Built-in Low-Latency Drum Preview Sampler**:
  - High-quality synthesized punchy 808/909-style Kick, Snare with noise snap, metallic Closed Hat, and sizzling Open Hat with choke support for instant standalone testing.
- 📊 **Real-Time Visualizer**:
  - Live 60 FPS oscilloscope displaying microphone audio waveform and transient trigger markers.
  - Interactive drum pads with velocity-responsive flash animations and manual audition clicks.
- 🛡️ **Real-Time Safe & Light CPU**: Zero heap allocations (`malloc`/`new`) and zero mutex locking in the audio callback.

---

## Architecture Overview

```
[ Microphone In ] 
        │
        ▼
[ DC Blocker & 45Hz High-Pass ]
        │
        ▼
[ Transient Onset Detector & Adaptive Noise Gate ]
        │
    (Triggered!)
        ├──> [ Micro-Feature Extractor (Centroid, Low/Mid/High Energy, ZCR, Decay) ]
        │           │
        │           ▼
        ├──> [ Classifier Engine (Calibrated Distance / Rule-based Heuristic) ]
        │           │
        │           ▼
        ├──> [ Dynamic Velocity Curve (dB -> MIDI 1-127) ]
        │           │
        │           ├──> [ MIDI Trigger Engine (Sample-Accurate Note-On / Note-Off) ] ──> DAW MIDI Track
        │           │
        │           └──> [ Internal Drum Preview Sampler ] ──> Audio Out (L/R)
        │
        └──> [ Lock-Free SPSC Telemetry Queue ] ──> 60 FPS GUI Oscilloscope & Pad Visualizer
```

---

## Building the Project

### Prerequisites
- CMake 3.22+
- C++20 compliant compiler (GCC 13+, Clang 15+, or MSVC 2022)
- Ninja build system

### Build Commands
```bash
# Configure build with Ninja
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Compile VST3, Standalone App, and Test Suite
cmake --build build --config Release

# Run automated DSP & Classifier unit tests
ctest --test-dir build --output-on-failure
```

---

## How to Use in Your DAW
1. Add **Teeth Drummer** as an audio effect on your microphone/audio input track.
2. Route the **MIDI Output** of the Teeth Drummer track to a drum instrument track (e.g. Superior Drummer, Addictive Drums, Steven Slate Drums, or DAW Drum Rack).
3. If using external drum instruments, toggle the **Audio Preview** button to mute the internal drum synth.
4. Adjust **Threshold** and **Noise Gate** so your teeth clicks trigger cleanly without false hits from breathing.
5. Click **"Learn Kick"**, tap your kick sound 5 times, and repeat for Snare, Closed Hat, and Open Hat to calibrate your personal mouth acoustic profile.

