#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <numbers>

#include "Common/Constants.h"
#include "DSP/TransientDetector.h"
#include "DSP/FeatureExtractor.h"
#include "Classifier/ClassifierEngine.h"
#include "Calibration/CalibrationManager.h"
#include "MIDI/MIDITriggerEngine.h"
#include "Synth/DrumSampler.h"

using namespace TeethDrummer;

void testTransientDetector()
{
    std::cout << "[TEST] Running TransientDetector test..." << std::endl;
    TransientDetector detector;
    detector.prepare(44100.0, 512);

    // 1. Feed 1000 samples of quiet background noise (should NOT trigger)
    float peakAmp = 0.0f;
    for (int i = 0; i < 1000; ++i)
    {
        float noise = ((i % 10) - 5) * 0.0001f;
        bool triggered = detector.processSample(noise, peakAmp);
        assert(!triggered && "False trigger on background noise!");
    }

    // 2. Feed a sharp transient impulse (should trigger immediately)
    bool detected = false;
    for (int i = 0; i < 100; ++i)
    {
        float sample = (i == 10) ? 0.8f : (i > 10 ? 0.8f * std::exp(-(i - 10) * 0.1f) : 0.0f);
        if (detector.processSample(sample, peakAmp))
        {
            detected = true;
            assert(peakAmp > 0.5f);
        }
    }
    assert(detected && "Failed to detect sharp transient impulse!");
    std::cout << "  -> TransientDetector passed!" << std::endl;
}

void testFeatureExtractorAndClassifier()
{
    std::cout << "[TEST] Running FeatureExtractor & Classifier test..." << std::endl;
    FeatureExtractor extractor;
    extractor.prepare(44100.0);
    ClassifierEngine engine;

    constexpr size_t N = DSPConfig::AnalysisWindowSize;
    std::vector<float> kickBurst(N, 0.0f);
    std::vector<float> hatBurst(N, 0.0f);

    // Synthetic Kick: 120Hz decaying sine (low frequency dominant)
    for (size_t i = 0; i < N; ++i)
    {
        float t = static_cast<float>(i) / 44100.0f;
        kickBurst[i] = std::sin(2.0f * static_cast<float>(std::numbers::pi) * 120.0f * t) * std::exp(-t * 100.0f);
    }

    // Synthetic Hi-Hat: 7000Hz decaying burst (high frequency dominant)
    for (size_t i = 0; i < N; ++i)
    {
        float t = static_cast<float>(i) / 44100.0f;
        hatBurst[i] = std::sin(2.0f * static_cast<float>(std::numbers::pi) * 7000.0f * t) * std::exp(-t * 400.0f);
    }

    auto kickFeats = extractor.extract(kickBurst.data(), N);
    auto hatFeats = extractor.extract(hatBurst.data(), N);

    assert(kickFeats.lowEnergyRatio > kickFeats.highEnergyRatio && "Kick low energy ratio failed");
    assert(hatFeats.highEnergyRatio > hatFeats.lowEnergyRatio && "Hat high energy ratio failed");

    float kickConf = 0.0f;
    float hatConf = 0.0f;
    auto kickPad = engine.classify(kickFeats, kickConf);
    auto hatPad = engine.classify(hatFeats, hatConf);

    assert(kickPad == DrumPad::Kick && "Kick classification failed");
    assert(hatPad == DrumPad::ClosedHat && "Hi-Hat classification failed");

    std::cout << "  -> Kick classified correctly (Conf: " << (kickConf * 100.0f) << "%)" << std::endl;
    std::cout << "  -> Hat classified correctly (Conf: " << (hatConf * 100.0f) << "%)" << std::endl;
    std::cout << "  -> FeatureExtractor & Classifier passed!" << std::endl;
}

void testMIDITriggerEngine()
{
    std::cout << "[TEST] Running MIDITriggerEngine test..." << std::endl;
    MIDITriggerEngine midiEngine;
    midiEngine.prepare(44100.0);

    juce::MidiBuffer midiBuffer;
    uint8_t vel = midiEngine.triggerHit(DrumPad::Kick, 0.8f, 5, midiBuffer);

    assert(vel >= 1 && vel <= 127 && "Invalid MIDI velocity");
    assert(!midiBuffer.isEmpty() && "MIDI buffer empty after triggerHit");

    int eventCount = 0;
    for (const auto meta : midiBuffer)
    {
        auto msg = meta.getMessage();
        assert(msg.isNoteOn() && "Expected Note-On event");
        assert(msg.getNoteNumber() == DefaultMidiNotes::Kick && "Incorrect MIDI Note number for Kick");
        ++eventCount;
    }
    assert(eventCount == 1 && "Expected exactly 1 MIDI event");
    std::cout << "  -> MIDITriggerEngine passed!" << std::endl;
}

void testDrumSampler()
{
    std::cout << "[TEST] Running DrumSampler test..." << std::endl;
    DrumSampler sampler;
    sampler.prepare(44100.0);

    sampler.trigger(DrumPad::Kick, 0.9f);
    sampler.trigger(DrumPad::Snare, 0.8f);
    sampler.trigger(DrumPad::ClosedHat, 0.7f);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    sampler.renderBlock(buffer, 0, 512);

    float maxAmp = buffer.getMagnitude(0, 512);
    assert(maxAmp > 0.001f && "DrumSampler output is silent");
    assert(maxAmp < 2.0f && "DrumSampler output clipped abnormally");

    std::cout << "  -> DrumSampler rendered successfully (Max Amp: " << maxAmp << ")" << std::endl;
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << " TEETH DRUMMER - DSP & ENGINE TEST SUITE" << std::endl;
    std::cout << "========================================" << std::endl;

    testTransientDetector();
    testFeatureExtractorAndClassifier();
    testMIDITriggerEngine();
    testDrumSampler();

    std::cout << "========================================" << std::endl;
    std::cout << " ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}

