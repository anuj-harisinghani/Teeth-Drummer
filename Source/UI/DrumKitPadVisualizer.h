#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/Constants.h"
#include <array>
#include <functional>

namespace TeethDrummer
{
    class DrumKitPadVisualizer : public juce::Component, public juce::Timer
    {
    public:
        using ManualTriggerCallback = std::function<void(DrumPad pad, float velocity)>;

        DrumKitPadVisualizer()
        {
            startTimerHz(30);
        }

        ~DrumKitPadVisualizer() override
        {
            stopTimer();
        }

        void setOnManualTrigger(ManualTriggerCallback callback)
        {
            onManualTrigger = std::move(callback);
        }

        void triggerPadFlash(DrumPad pad, float velocity)
        {
            const size_t idx = static_cast<size_t>(pad);
            if (idx < padFlashes.size())
            {
                padFlashes[idx] = std::max(0.4f, velocity);
                repaint();
            }
        }

        void setMidiNote(DrumPad pad, uint8_t note)
        {
            const size_t idx = static_cast<size_t>(pad);
            if (idx < midiNotes.size())
            {
                midiNotes[idx] = note;
                repaint();
            }
        }

        void timerCallback() override
        {
            bool needsRepaint = false;
            for (auto& flash : padFlashes)
            {
                if (flash > 0.0f)
                {
                    flash -= 0.08f;
                    if (flash < 0.0f) flash = 0.0f;
                    needsRepaint = true;
                }
            }

            if (needsRepaint)
                repaint();
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            const float padGap = 10.0f;
            const float padWidth = (bounds.getWidth() - padGap * 3.0f) / 4.0f;
            const float padHeight = bounds.getHeight();

            for (size_t i = 0; i < 4; ++i)
            {
                const auto pad = static_cast<DrumPad>(i);
                const auto padRect = juce::Rectangle<float>(
                    bounds.getX() + static_cast<float>(i) * (padWidth + padGap),
                    bounds.getY(),
                    padWidth,
                    padHeight
                );

                juce::Colour baseColor, accentColor;
                switch (pad)
                {
                    case DrumPad::Kick:
                        baseColor = juce::Colour(0xff2d1515);
                        accentColor = juce::Colour(0xffef4444);
                        break;
                    case DrumPad::Snare:
                        baseColor = juce::Colour(0xff152238);
                        accentColor = juce::Colour(0xff3b82f6);
                        break;
                    case DrumPad::ClosedHat:
                        baseColor = juce::Colour(0xff332b12);
                        accentColor = juce::Colour(0xffeab308);
                        break;
                    case DrumPad::OpenHat:
                        baseColor = juce::Colour(0xff281636);
                        accentColor = juce::Colour(0xffa855f7);
                        break;
                    default:
                        break;
                }

                // Blend flash highlight
                const float flash = padFlashes[i];
                const auto blendedFill = baseColor.interpolatedWith(accentColor, flash * 0.8f);

                g.setColour(blendedFill);
                g.fillRoundedRectangle(padRect, 8.0f);

                // Border glow on hit
                g.setColour(accentColor.withAlpha(0.4f + flash * 0.6f));
                g.drawRoundedRectangle(padRect, 8.0f, 1.5f + flash * 2.0f);

                // Pad Name
                g.setColour(juce::Colours::white);
                g.setFont(juce::Font(16.0f, juce::Font::bold));
                g.drawText(juce::String(getDrumPadName(pad).data()),
                           padRect.withHeight(padRect.getHeight() * 0.55f),
                           juce::Justification::centred);

                // MIDI Note info
                g.setColour(juce::Colour(0xff9ca3af));
                g.setFont(12.0f);
                const juce::String noteStr = "MIDI Note: " + juce::String(midiNotes[i]);
                g.drawText(noteStr, padRect, juce::Justification::centred);
            }
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            auto bounds = getLocalBounds().toFloat();
            const float padGap = 10.0f;
            const float padWidth = (bounds.getWidth() - padGap * 3.0f) / 4.0f;

            for (size_t i = 0; i < 4; ++i)
            {
                const auto padRect = juce::Rectangle<float>(
                    bounds.getX() + static_cast<float>(i) * (padWidth + padGap),
                    bounds.getY(),
                    padWidth,
                    bounds.getHeight()
                );

                if (padRect.contains(e.position))
                {
                    const auto pad = static_cast<DrumPad>(i);
                    triggerPadFlash(pad, 1.0f);
                    if (onManualTrigger)
                        onManualTrigger(pad, 0.9f);
                    break;
                }
            }
        }

    private:
        std::array<float, 4> padFlashes{0.0f, 0.0f, 0.0f, 0.0f};
        std::array<uint8_t, 4> midiNotes{DefaultMidiNotes::Kick, DefaultMidiNotes::Snare, DefaultMidiNotes::ClosedHat, DefaultMidiNotes::OpenHat};
        ManualTriggerCallback onManualTrigger;
    };
}

