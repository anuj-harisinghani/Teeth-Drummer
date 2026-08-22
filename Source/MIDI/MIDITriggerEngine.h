#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../Common/Constants.h"
#include "VelocityCurve.h"
#include <array>

namespace TeethDrummer
{
    struct ActiveNote
    {
        bool isActive{false};
        uint8_t midiNote{0};
        uint8_t midiChannel{1};
        int remainingSamples{0};
    };

    class MIDITriggerEngine
    {
    public:
        MIDITriggerEngine();

        void prepare(double sampleRate);
        void reset();

        // Sets MIDI note assignment for a specific pad
        void setPadNote(DrumPad pad, uint8_t midiNote);
        uint8_t getPadNote(DrumPad pad) const noexcept;

        void setMidiChannel(uint8_t channel); // 1 to 16
        uint8_t getMidiChannel() const noexcept { return midiChannel; }

        void setGateDurationMs(float durationMs);

        VelocityCurve& getVelocityCurve() noexcept { return velocityCurve; }
        const VelocityCurve& getVelocityCurve() const noexcept { return velocityCurve; }

        // Triggers a pad hit at a specific sample offset in the current block
        uint8_t triggerHit(DrumPad pad, float peakAmp, int sampleOffset, juce::MidiBuffer& midiMessages);

        // Processes active note countdowns and appends any pending Note-Off events to midiMessages
        void processBlock(int numSamplesInBlock, juce::MidiBuffer& midiMessages);

    private:
        double currentSampleRate{DSPConfig::DefaultSampleRate};
        uint8_t midiChannel{10}; // Default to Channel 10 (standard MIDI percussion channel)
        int gateSamples{2205};   // ~50ms @ 44.1kHz

        VelocityCurve velocityCurve;
        std::array<uint8_t, static_cast<size_t>(DrumPad::Count)> padNoteMap{};
        std::array<ActiveNote, static_cast<size_t>(DrumPad::Count)> activeNotes{};
    };
}

