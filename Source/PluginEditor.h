#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/OscilloscopeComponent.h"
#include "UI/DrumKitPadVisualizer.h"
#include "UI/SensitivityControls.h"
#include "UI/CalibrationWizardView.h"

namespace TeethDrummer
{
    class TeethDrummerAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
    {
    public:
        explicit TeethDrummerAudioProcessorEditor(TeethDrummerAudioProcessor&);
        ~TeethDrummerAudioProcessorEditor() override;

        void paint(juce::Graphics&) override;
        void resized() override;
        void timerCallback() override;

    private:
        TeethDrummerAudioProcessor& processorRef;

        OscilloscopeComponent oscilloscope;
        DrumKitPadVisualizer padVisualizer;
        SensitivityControls controls;
        CalibrationWizardView calibrationWizard;

        juce::Label headerLabel;
        juce::Label versionLabel;
        juce::Label subtitleLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TeethDrummerAudioProcessorEditor)
    };
}

