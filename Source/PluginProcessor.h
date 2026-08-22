#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Common/Constants.h"
#include "Common/LockFreeQueue.h"
#include "DSP/CircularAudioBuffer.h"
#include "DSP/TransientDetector.h"
#include "DSP/FeatureExtractor.h"
#include "Classifier/ClassifierEngine.h"
#include "MIDI/MIDITriggerEngine.h"
#include "Synth/DrumSampler.h"
#include "Calibration/CalibrationManager.h"

namespace TeethDrummer
{
    class TeethDrummerAudioProcessor : public juce::AudioProcessor
    {
    public:
        TeethDrummerAudioProcessor();
        ~TeethDrummerAudioProcessor() override;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;

        bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override;

        const juce::String getName() const override;

        bool acceptsMidi() const override;
        bool producesMidi() const override;
        bool isMidiEffect() const override;
        double getTailLengthSeconds() const override;

        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram(int index) override;
        const juce::String getProgramName(int index) override;
        void changeProgramName(int index, const juce::String& newName) override;

        void getStateInformation(juce::MemoryBlock& destData) override;
        void setStateInformation(const void* data, int sizeInBytes) override;

        // Accessors for UI
        juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
        ClassifierEngine& getClassifierEngine() noexcept { return classifierEngine; }
        CalibrationManager& getCalibrationManager() noexcept { return calibrationManager; }
        MIDITriggerEngine& getMIDIEngine() noexcept { return midiEngine; }
        DrumSampler& getDrumSampler() noexcept { return drumSampler; }

        LockFreeQueue<TriggerTelemetryEvent, 64>& getTelemetryQueue() noexcept { return telemetryQueue; }
        LockFreeQueue<float, 4096>& getScopeQueue() noexcept { return scopeQueue; }

        void triggerManualPad(DrumPad pad, float velocity);

    private:
        juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
        void updateParameters();

        juce::AudioProcessorValueTreeState apvts;

        // Core DSP & Classification Engines
        CircularAudioBuffer historyBuffer;
        TransientDetector transientDetector;
        FeatureExtractor featureExtractor;
        ClassifierEngine classifierEngine;
        MIDITriggerEngine midiEngine;
        DrumSampler drumSampler;
        CalibrationManager calibrationManager;

        // Lock-free telemetry queues for UI
        LockFreeQueue<TriggerTelemetryEvent, 64> telemetryQueue;
        LockFreeQueue<float, 4096> scopeQueue;

        std::array<float, DSPConfig::AnalysisWindowSize> analysisBuffer{};
        int scopeSubsampleCounter{0};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TeethDrummerAudioProcessor)
    };
}

