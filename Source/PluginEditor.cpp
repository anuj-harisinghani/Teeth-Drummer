#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace TeethDrummer
{
    TeethDrummerAudioProcessorEditor::TeethDrummerAudioProcessorEditor(TeethDrummerAudioProcessor& p)
        : AudioProcessorEditor(&p),
          processorRef(p),
          oscilloscope(p.getScopeQueue()),
          controls(p.getAPVTS()),
          calibrationWizard(p.getCalibrationManager())
    {
        headerLabel.setText("TEETH DRUMMER", juce::dontSendNotification);
        headerLabel.setFont(juce::Font(22.0f, juce::Font::bold));
        headerLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(headerLabel);

        subtitleLabel.setText("Real-Time Oral Percussion Audio-to-MIDI Trigger", juce::dontSendNotification);
        subtitleLabel.setFont(juce::Font(12.0f, juce::Font::plain));
        subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
        addAndMakeVisible(subtitleLabel);

        addAndMakeVisible(oscilloscope);
        addAndMakeVisible(padVisualizer);
        addAndMakeVisible(controls);
        addAndMakeVisible(calibrationWizard);

        padVisualizer.setOnManualTrigger([this](DrumPad pad, float vel) {
            processorRef.triggerManualPad(pad, vel);
        });

        // Set plugin window dimensions
        setSize(780, 620);
        setResizable(true, true);
        setResizeLimits(700, 560, 1200, 900);

        startTimerHz(60);
    }

    TeethDrummerAudioProcessorEditor::~TeethDrummerAudioProcessorEditor()
    {
        stopTimer();
    }

    void TeethDrummerAudioProcessorEditor::timerCallback()
    {
        // Poll telemetry queue for live hits
        TriggerTelemetryEvent event;
        while (processorRef.getTelemetryQueue().pop(event))
        {
            padVisualizer.triggerPadFlash(event.pad, event.velocity);
            oscilloscope.addHitMarker(event.pad, event.velocity);
            calibrationWizard.updateTelemetry(event);
        }
    }

    void TeethDrummerAudioProcessorEditor::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour(0xff0f0f14)); // Deep modern dark background

        // Header bottom accent line
        g.setColour(juce::Colour(0xff27272a));
        g.drawHorizontalLine(55, 0.0f, static_cast<float>(getWidth()));
    }

    void TeethDrummerAudioProcessorEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(16);

        // Header
        auto headerArea = bounds.removeFromTop(44);
        headerLabel.setBounds(headerArea.removeFromTop(24));
        subtitleLabel.setBounds(headerArea);

        bounds.removeFromTop(12);

        // 1. Oscilloscope View
        oscilloscope.setBounds(bounds.removeFromTop(130));
        bounds.removeFromTop(12);

        // 2. Drum Kit Visualizer Pads
        padVisualizer.setBounds(bounds.removeFromTop(110));
        bounds.removeFromTop(12);

        // 3. Sensitivity & DSP Controls
        controls.setBounds(bounds.removeFromTop(110));
        bounds.removeFromTop(12);

        // 4. Calibration Wizard & Telemetry View
        calibrationWizard.setBounds(bounds);
    }
}

