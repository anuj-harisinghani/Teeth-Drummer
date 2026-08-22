#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/LockFreeQueue.h"
#include <vector>
#include <deque>

namespace TeethDrummer
{
    class OscilloscopeComponent : public juce::Component, public juce::Timer
    {
    public:
        OscilloscopeComponent(LockFreeQueue<float, 4096>& scopeQ)
            : scopeQueue(scopeQ)
        {
            waveformHistory.resize(512, 0.0f);
            startTimerHz(30);
        }

        ~OscilloscopeComponent() override
        {
            stopTimer();
        }

        void addHitMarker(DrumPad pad, float velocity)
        {
            HitMarker m;
            m.pad = pad;
            m.velocity = velocity;
            m.xPos = static_cast<float>(waveformHistory.size() - 1);
            m.opacity = 1.0f;
            markers.push_back(m);
        }

        void timerCallback() override
        {
            float sample = 0.0f;
            bool newSamples = false;

            while (scopeQueue.pop(sample))
            {
                waveformHistory.pop_front();
                waveformHistory.push_back(sample);
                newSamples = true;
            }

            // Update hit markers
            for (auto& m : markers)
            {
                m.opacity -= 0.04f;
            }
            std::erase_if(markers, [](const HitMarker& m) { return m.opacity <= 0.0f; });

            if (newSamples || !markers.empty())
                repaint();
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();

            // Background
            g.setColour(juce::Colour(0xff18181f));
            g.fillRoundedRectangle(bounds, 6.0f);

            // Grid lines
            g.setColour(juce::Colour(0xff2a2a38));
            const float midY = bounds.getCentreY();
            g.drawHorizontalLine(static_cast<int>(midY), bounds.getX(), bounds.getRight());

            // Waveform path
            juce::Path path;
            const float numPoints = static_cast<float>(waveformHistory.size());
            const float xStep = bounds.getWidth() / numPoints;

            for (size_t i = 0; i < waveformHistory.size(); ++i)
            {
                const float x = bounds.getX() + static_cast<float>(i) * xStep;
                const float y = midY - (waveformHistory[i] * bounds.getHeight() * 0.45f);

                if (i == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }

            g.setColour(juce::Colour(0xff4ade80)); // Neon Green
            g.strokePath(path, juce::PathStrokeType(1.5f));

            // Draw transient hit markers
            for (const auto& m : markers)
            {
                juce::Colour padColour = juce::Colours::white;
                switch (m.pad)
                {
                    case DrumPad::Kick:      padColour = juce::Colour(0xffef4444); break; // Red
                    case DrumPad::Snare:     padColour = juce::Colour(0xff3b82f6); break; // Blue
                    case DrumPad::ClosedHat: padColour = juce::Colour(0xffeab308); break; // Yellow
                    case DrumPad::OpenHat:   padColour = juce::Colour(0xffa855f7); break; // Purple
                    default: break;
                }

                g.setColour(padColour.withAlpha(m.opacity));
                const float markerX = bounds.getX() + m.xPos * xStep;
                g.drawVerticalLine(static_cast<int>(markerX), bounds.getY(), bounds.getBottom());

                g.setFont(11.0f);
                g.drawText(juce::String(getDrumPadName(m.pad).data()), 
                           static_cast<int>(markerX - 30), static_cast<int>(bounds.getY() + 4), 60, 16, 
                           juce::Justification::centred);
            }

            // Border
            g.setColour(juce::Colour(0xff374151));
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
        }

    private:
        struct HitMarker
        {
            DrumPad pad{DrumPad::None};
            float velocity{1.0f};
            float xPos{0.0f};
            float opacity{1.0f};
        };

        LockFreeQueue<float, 4096>& scopeQueue;
        std::deque<float> waveformHistory;
        std::vector<HitMarker> markers;
    };
}

