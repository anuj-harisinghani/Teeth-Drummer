#include "MIDITriggerEngine.h"
#include <algorithm>

namespace TeethDrummer
{
    MIDITriggerEngine::MIDITriggerEngine()
    {
        padNoteMap[static_cast<size_t>(DrumPad::Kick)]      = DefaultMidiNotes::Kick;
        padNoteMap[static_cast<size_t>(DrumPad::Snare)]     = DefaultMidiNotes::Snare;
        padNoteMap[static_cast<size_t>(DrumPad::ClosedHat)] = DefaultMidiNotes::ClosedHat;
        padNoteMap[static_cast<size_t>(DrumPad::OpenHat)]   = DefaultMidiNotes::OpenHat;

        prepare(DSPConfig::DefaultSampleRate);
    }

    void MIDITriggerEngine::prepare(double sampleRate)
    {
        currentSampleRate = (sampleRate > 0.0) ? sampleRate : DSPConfig::DefaultSampleRate;
        setGateDurationMs(DSPConfig::DefaultNoteDurationMs);
        reset();
    }

    void MIDITriggerEngine::reset()
    {
        for (auto& note : activeNotes)
        {
            note.isActive = false;
            note.remainingSamples = 0;
        }
    }

    void MIDITriggerEngine::setPadNote(DrumPad pad, uint8_t midiNote)
    {
        if (pad != DrumPad::None && static_cast<size_t>(pad) < padNoteMap.size())
        {
            padNoteMap[static_cast<size_t>(pad)] = std::clamp<uint8_t>(midiNote, 0, 127);
        }
    }

    uint8_t MIDITriggerEngine::getPadNote(DrumPad pad) const noexcept
    {
        if (pad != DrumPad::None && static_cast<size_t>(pad) < padNoteMap.size())
            return padNoteMap[static_cast<size_t>(pad)];
        return 0;
    }

    void MIDITriggerEngine::setMidiChannel(uint8_t channel)
    {
        midiChannel = std::clamp<uint8_t>(channel, 1, 16);
    }

    void MIDITriggerEngine::setGateDurationMs(float durationMs)
    {
        const float clamped = std::clamp(durationMs, 10.0f, 500.0f);
        gateSamples = static_cast<int>(clamped * 0.001f * currentSampleRate);
    }

    uint8_t MIDITriggerEngine::triggerHit(DrumPad pad, float peakAmp, int sampleOffset, juce::MidiBuffer& midiMessages)
    {
        if (pad == DrumPad::None || static_cast<size_t>(pad) >= padNoteMap.size())
            return 0;

        const size_t padIdx = static_cast<size_t>(pad);
        const uint8_t noteNum = padNoteMap[padIdx];
        const uint8_t velocity = velocityCurve.mapToVelocity(peakAmp);

        // If this note is already active, send a fast Note-Off first to re-trigger cleanly
        if (activeNotes[padIdx].isActive)
        {
            const auto noteOff = juce::MidiMessage::noteOff(activeNotes[padIdx].midiChannel, activeNotes[padIdx].midiNote, (uint8_t)0);
            midiMessages.addEvent(noteOff, std::max(0, sampleOffset - 1));
        }

        // Send Note-On
        const auto noteOn = juce::MidiMessage::noteOn(midiChannel, noteNum, velocity);
        midiMessages.addEvent(noteOn, sampleOffset);

        // Track active note for subsequent Note-Off release
        activeNotes[padIdx].isActive = true;
        activeNotes[padIdx].midiNote = noteNum;
        activeNotes[padIdx].midiChannel = midiChannel;
        activeNotes[padIdx].remainingSamples = gateSamples;

        return velocity;
    }

    void MIDITriggerEngine::processBlock(int numSamplesInBlock, juce::MidiBuffer& midiMessages)
    {
        for (auto& note : activeNotes)
        {
            if (!note.isActive)
                continue;

            if (note.remainingSamples <= numSamplesInBlock)
            {
                // Note expires inside this buffer block
                const int offset = std::clamp(note.remainingSamples, 0, numSamplesInBlock - 1);
                const auto noteOff = juce::MidiMessage::noteOff(note.midiChannel, note.midiNote, (uint8_t)0);
                midiMessages.addEvent(noteOff, offset);

                note.isActive = false;
                note.remainingSamples = 0;
            }
            else
            {
                note.remainingSamples -= numSamplesInBlock;
            }
        }
    }
}

