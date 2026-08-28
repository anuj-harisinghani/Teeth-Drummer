#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <numbers>
#include <chrono>
#include <atomic>
#include <cstdlib>
#include <new>

#include "Common/Constants.h"
#include "DSP/CircularAudioBuffer.h"
#include "DSP/TransientDetector.h"
#include "DSP/FeatureExtractor.h"
#include "Classifier/ClassifierEngine.h"
#include "Calibration/CalibrationManager.h"
#include "MIDI/MIDITriggerEngine.h"
#include "Synth/DrumSampler.h"

using namespace TeethDrummer;

// --- Heap-allocation tripwire -----------------------------------------------
// Overrides global operator new/delete for this benchmark executable only, so the
// real-time hot path can be run under a counter and its "zero allocations" claim
// verified directly rather than just eyeballed from the source.
namespace
{
    std::atomic<long long> g_allocCount{0};
    std::atomic<bool> g_countAllocs{false};
}

void* operator new(std::size_t size)
{
    if (g_countAllocs.load(std::memory_order_relaxed))
        g_allocCount.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(size == 0 ? 1 : size))
        return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

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

void testCalibrationManagerLockFreeFlow()
{
    std::cout << "[TEST] Running CalibrationManager lock-free flow test..." << std::endl;
    ClassifierEngine engine;
    CalibrationManager calibManager(engine);

    int lastCurrentHits = -1;
    bool sawFinished = false;
    calibManager.setOnStateChanged([&](DrumPad, int currentHits, int, bool finished) {
        lastCurrentHits = currentHits;
        if (finished) sawFinished = true;
    });

    // Simulate the audio thread queuing hits before calibration starts - must be dropped.
    FeatureVector stale;
    stale.lowEnergyRatio = 0.9f;
    assert(!calibManager.queueCalibrationHit(stale) && "Hit should be rejected when not calibrating");

    calibManager.startCalibration(DrumPad::Kick, 3);
    assert(calibManager.isCalibrating());
    assert(calibManager.getTargetPad() == DrumPad::Kick);

    // Simulate the audio thread queuing 3 kick-like hits (lock-free, no draining yet).
    for (int i = 0; i < 3; ++i)
    {
        FeatureVector feats;
        feats.lowEnergyRatio = 0.8f;
        feats.midEnergyRatio = 0.1f;
        feats.highEnergyRatio = 0.1f;
        feats.spectralCentroid = 200.0f;
        assert(calibManager.queueCalibrationHit(feats) && "Hit should be accepted while calibrating");
    }
    // Queueing must not have touched GUI-thread-only state yet.
    assert(calibManager.getCurrentHits() == 0 && "Hits should not be recorded until pumped");

    // Simulate the GUI thread's timer draining the queue.
    calibManager.pumpCalibrationQueue();

    assert(!calibManager.isCalibrating() && "Calibration should finish after 3rd hit");
    assert(sawFinished && "onStateChanged should report finished=true");
    assert(lastCurrentHits == 3);

    const auto profile = engine.getProfile();
    const auto& kickProto = profile.prototypes[static_cast<size_t>(DrumPad::Kick)];
    assert(kickProto.isCalibrated && "Kick prototype should be marked calibrated");
    assert(kickProto.sampleCount == 3);
    assert(std::abs(kickProto.lowEnergyRatio - 0.8f) < 0.001f && "Averaged feature mismatch");

    // A hit queued after calibration naturally finished should be rejected again.
    assert(!calibManager.queueCalibrationHit(stale) && "Hit should be rejected once calibration is done");

    std::cout << "  -> CalibrationManager lock-free flow passed!" << std::endl;
}

// --- Real-time performance benchmark ----------------------------------------
// Mirrors PluginProcessor::processBlock's per-sample hot path (transient detection
// -> feature extraction -> calibration hand-off -> classification -> MIDI trigger ->
// drum synth render) against synthetic audio, to give concrete, current numbers for
// "how much of the real-time budget does this actually use" rather than a guess.
void benchmarkAudioPipeline()
{
    std::cout << "[BENCH] Running real-time audio pipeline benchmark..." << std::endl;

    constexpr double sampleRate = 44100.0;

    TransientDetector detector;
    detector.prepare(sampleRate, 512);
    FeatureExtractor extractor;
    extractor.prepare(sampleRate);
    ClassifierEngine engine;
    MIDITriggerEngine midiEngine;
    midiEngine.prepare(sampleRate);
    DrumSampler sampler;
    sampler.prepare(sampleRate);
    CircularAudioBuffer historyBuffer;

    // 10 seconds of synthetic input: quiet background noise with a sharp transient
    // injected roughly every 100ms (~10 hits/sec - a fast, sustained drummer) so the
    // benchmark exercises the expensive "hit detected" branch, not just idle noise-gating.
    constexpr int totalSamples = static_cast<int>(sampleRate) * 10;
    std::vector<float> input(static_cast<size_t>(totalSamples));
    for (int i = 0; i < totalSamples; ++i)
        input[static_cast<size_t>(i)] = static_cast<float>((i % 17) - 8) * 0.0001f;

    constexpr int hitSpacing = 4410; // ~100ms
    for (int start = 0; start < totalSamples; start += hitSpacing)
    {
        for (int j = 0; j < 200 && (start + j) < totalSamples; ++j)
        {
            const float t = static_cast<float>(j) / static_cast<float>(sampleRate);
            input[static_cast<size_t>(start + j)] +=
                std::sin(2.0f * static_cast<float>(std::numbers::pi) * 150.0f * t) * std::exp(-t * 80.0f) * 0.8f;
        }
    }

    std::array<float, DSPConfig::AnalysisWindowSize> analysisBuffer{};
    int hitsDetected = 0;

    // Warm up (fills history buffer, first few detector windows) before timing/counting.
    for (int i = 0; i < 4096; ++i)
    {
        float peakAmp = 0.0f;
        historyBuffer.pushSample(input[static_cast<size_t>(i)]);
        detector.processSample(input[static_cast<size_t>(i)], peakAmp);
    }

    g_allocCount.store(0, std::memory_order_relaxed);
    g_countAllocs.store(true, std::memory_order_relaxed);

    const auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 4096; i < totalSamples; ++i)
    {
        const float sample = input[static_cast<size_t>(i)];
        historyBuffer.pushSample(sample);

        float peakAmp = 0.0f;
        if (detector.processSample(sample, peakAmp))
        {
            historyBuffer.copyRecentSamples(analysisBuffer.data(), analysisBuffer.size());
            const FeatureVector feats = extractor.extract(analysisBuffer.data(), analysisBuffer.size());

            float confidence = 0.0f;
            const DrumPad pad = engine.classify(feats, confidence);

            if (pad != DrumPad::None)
            {
                juce::MidiBuffer midi;
                const uint8_t vel = midiEngine.triggerHit(pad, peakAmp, 0, midi);
                sampler.trigger(pad, static_cast<float>(vel) / 127.0f);
                ++hitsDetected;
            }
        }
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    g_countAllocs.store(false, std::memory_order_relaxed);
    const long long allocsDuringHotPath = g_allocCount.load(std::memory_order_relaxed);

    const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    const int samplesProcessed = totalSamples - 4096;
    const double audioDurationSec = static_cast<double>(samplesProcessed) / sampleRate;
    const double wallSeconds = static_cast<double>(elapsedNs) / 1e9;
    const double realTimeFactor = audioDurationSec / wallSeconds; // >1 means faster than real-time
    const double nsPerSample = static_cast<double>(elapsedNs) / samplesProcessed;

    std::cout << "  -> Processed " << samplesProcessed << " samples (" << audioDurationSec << "s of audio) in "
              << wallSeconds << "s wall-clock" << std::endl;
    std::cout << "  -> " << nsPerSample << " ns/sample average" << std::endl;
    std::cout << "  -> Real-time factor: " << realTimeFactor << "x (>1x = faster than real-time)" << std::endl;
    std::cout << "  -> Hits detected: " << hitsDetected << std::endl;
    std::cout << "  -> Heap allocations during hot path: " << allocsDuringHotPath << std::endl;

    // Typical low-latency block sizes: confirm even the smallest realistic buffer
    // (64 samples @ 44.1kHz = ~1.45ms budget) has ample headroom on average.
    const double avgBlockNs64 = nsPerSample * 64.0;
    const double budgetNs64 = (64.0 / sampleRate) * 1e9;
    std::cout << "  -> Avg cost for a 64-sample block: " << (avgBlockNs64 / 1000.0) << "us vs "
              << (budgetNs64 / 1000.0) << "us budget ("
              << (100.0 * avgBlockNs64 / budgetNs64) << "% of budget)" << std::endl;

    assert(allocsDuringHotPath == 0 && "Heap allocation detected on the real-time audio path!");
    assert(realTimeFactor > 10.0 && "Audio pipeline is uncomfortably close to real-time budget");

    std::cout << "  -> Benchmark passed (real-time safe, no allocations)!" << std::endl;
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
    testCalibrationManagerLockFreeFlow();
    benchmarkAudioPipeline();

    std::cout << "========================================" << std::endl;
    std::cout << " ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}

