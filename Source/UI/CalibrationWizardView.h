#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/Constants.h"
#include "../Common/LockFreeQueue.h"
#include "../Calibration/CalibrationManager.h"
#include <array>

namespace TeethDrummer
{
    class CalibrationWizardView : public juce::Component
    {
    public:
        CalibrationWizardView(CalibrationManager& calManager)
            : calibrationManager(calManager)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                const auto pad = static_cast<DrumPad>(i);
                auto& btn = learnButtons[i];
                btn.setButtonText("Learn " + juce::String(getDrumPadName(pad).data()));
                btn.onClick = [this, pad]() {
                    if (calibrationManager.isCalibrating() && calibrationManager.getTargetPad() == pad)
                        calibrationManager.cancelCalibration();
                    else
                        calibrationManager.startCalibration(pad, 5);
                };
                addAndMakeVisible(btn);
            }

            resetAllButton.setButtonText("Reset Calibration");
            resetAllButton.onClick = [this]() {
                calibrationManager.resetAll();
            };
            addAndMakeVisible(resetAllButton);

            statusLabel.setText("Calibration: Factory Default Acoustic Profile", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
            statusLabel.setFont(juce::Font(13.0f, juce::Font::plain));
            addAndMakeVisible(statusLabel);

            telemetryLabel.setText("Last Hit: None", juce::dontSendNotification);
            telemetryLabel.setColour(juce::Label::textColourId, juce::Colour(0xff6b7280));
            telemetryLabel.setFont(juce::Font(11.0f, juce::Font::plain));
            addAndMakeVisible(telemetryLabel);

            calibrationManager.setOnStateChanged([this](DrumPad pad, int cur, int target, bool finished) {
                juce::MessageManager::callAsync([this, pad, cur, target, finished]() {
                    updateStatus(pad, cur, target, finished);
                });
            });
        }

        void updateTelemetry(const TriggerTelemetryEvent& event)
        {
            const juce::String padName(getDrumPadName(event.pad).data());
            const juce::String text = "Last Hit: " + padName + 
                                      " | Vel: " + juce::String(event.midiVelocity) + 
                                      " | Centroid: " + juce::String(static_cast<int>(event.spectralCentroid)) + "Hz" +
                                      " | Low: " + juce::String(static_cast<int>(event.lowEnergyRatio * 100.0f)) + "%" +
                                      " | Mid: " + juce::String(static_cast<int>(event.midEnergyRatio * 100.0f)) + "%" +
                                      " | High: " + juce::String(static_cast<int>(event.highEnergyRatio * 100.0f)) + "%" +
                                      " | Conf: " + juce::String(static_cast<int>(event.confidence * 100.0f)) + "%";
            telemetryLabel.setText(text, juce::dontSendNotification);
        }

        void updateStatus(DrumPad pad, int cur, int target, bool finished)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                const auto p = static_cast<DrumPad>(i);
                if (calibrationManager.isCalibrating() && calibrationManager.getTargetPad() == p)
                {
                    learnButtons[i].setButtonText("Cancel (" + juce::String(cur) + "/" + juce::String(target) + ")");
                }
                else
                {
                    learnButtons[i].setButtonText("Learn " + juce::String(getDrumPadName(p).data()));
                }
            }

            if (finished)
            {
                statusLabel.setText("Calibration complete for " + juce::String(getDrumPadName(pad).data()) + "!", juce::dontSendNotification);
                statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80));
            }
            else if (calibrationManager.isCalibrating())
            {
                statusLabel.setText("Click your " + juce::String(getDrumPadName(pad).data()) + " teeth hit (" + juce::String(cur) + "/" + juce::String(target) + " recorded)...", juce::dontSendNotification);
                statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xfffacc15));
            }
            else
            {
                statusLabel.setText("Profile Ready. Click 'Learn' to personalize any pad.", juce::dontSendNotification);
                statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            auto topRow = bounds.removeFromTop(32);
            
            const int btnWidth = (topRow.getWidth() - 40) / 5;
            for (size_t i = 0; i < 4; ++i)
            {
                learnButtons[i].setBounds(topRow.removeFromLeft(btnWidth));
                topRow.removeFromLeft(8);
            }
            resetAllButton.setBounds(topRow);

            bounds.removeFromTop(6);
            statusLabel.setBounds(bounds.removeFromTop(20));
            telemetryLabel.setBounds(bounds.removeFromTop(18));
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0xff1f2937));
            g.fillRoundedRectangle(bounds, 6.0f);
            g.setColour(juce::Colour(0xff374151));
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
        }

    private:
        CalibrationManager& calibrationManager;
        std::array<juce::TextButton, 4> learnButtons;
        juce::TextButton resetAllButton;
        juce::Label statusLabel;
        juce::Label telemetryLabel;
    };
}

