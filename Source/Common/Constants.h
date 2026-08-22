#pragma once

#include <cstdint>
#include <string_view>
#include <array>

namespace TeethDrummer
{
    // Four core drum pad categories
    enum class DrumPad : uint8_t
    {
        Kick = 0,
        Snare = 1,
        ClosedHat = 2,
        OpenHat = 3,
        Count = 4,
        None = 255
    };

    inline constexpr std::array<DrumPad, 4> AllDrumPads = {
        DrumPad::Kick,
        DrumPad::Snare,
        DrumPad::ClosedHat,
        DrumPad::OpenHat
    };

    inline constexpr std::string_view getDrumPadName(DrumPad pad) noexcept
    {
        switch (pad)
        {
            case DrumPad::Kick:      return "Kick";
            case DrumPad::Snare:     return "Snare";
            case DrumPad::ClosedHat: return "Closed Hat";
            case DrumPad::OpenHat:   return "Open Hat";
            default:                 return "None";
        }
    }

    // Default General MIDI (GM) Drum Map Notes
    namespace DefaultMidiNotes
    {
        inline constexpr uint8_t Kick      = 36; // C1 (Acoustic Bass Drum)
        inline constexpr uint8_t Snare     = 38; // D1 (Acoustic Snare)
        inline constexpr uint8_t ClosedHat = 42; // F#1 (Closed Hi-Hat)
        inline constexpr uint8_t OpenHat   = 46; // A#1 (Open Hi-Hat)
    }

    // DSP Configuration Limits
    namespace DSPConfig
    {
        inline constexpr float DefaultSampleRate = 44100.0f;
        inline constexpr int   MaxBlockSize       = 2048;
        
        // Window size for micro-feature extraction (e.g. ~128 samples = 2.9ms @ 44.1kHz)
        inline constexpr int   AnalysisWindowSize = 128;
        
        // Circular buffer size to hold history and transient lookback
        inline constexpr int   HistoryBufferSize  = 1024;
        
        // Minimum retrigger dead time in milliseconds (prevents double-triggering)
        inline constexpr float DefaultRetriggerMs = 30.0f;
        inline constexpr float MinRetriggerMs     = 10.0f;
        inline constexpr float MaxRetriggerMs     = 100.0f;

        // Default MIDI Note duration in milliseconds
        inline constexpr float DefaultNoteDurationMs = 50.0f;

        // Frequency band split boundaries (Hz)
        inline constexpr float LowBandCutoff  = 350.0f;
        inline constexpr float HighBandCutoff = 3500.0f;
    }
}

