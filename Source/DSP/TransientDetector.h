#pragma once

#include <cstdint>
#include <algorithm>
#include "../Common/Constants.h"
#include "BiquadFilter.h"
#include "EnvelopeFollower.h"
#include "CircularAudioBuffer.h"

namespace TeethDrummer
{
    struct OnsetEvent
    {
        bool triggered{false};
        int sampleOffset{0};       // Sample position within current block
        float peakAmplitude{0.0f};  // Raw peak absolute amplitude
        float energyDerivative{0.0f};
    };

    class TransientDetector
    {
    public:
        TransientDetector();

        void prepare(double sampleRate, int maxBlockSize);
        void reset();

        // Process a single audio sample; returns true if an onset is detected on this sample
        bool processSample(float inputSample, float& outPeakAmp);

        // Parameters
        void setThreshold(float normalizedThreshold); // 0.0 to 1.0
        void setSensitivity(float sensitivity);       // 0.1 to 10.0
        void setRetriggerTimeMs(float ms);            // 10 to 100 ms
        void setNoiseGateFloor(float gateThreshold);   // linear amplitude floor

        float getNoiseFloor() const noexcept { return noiseFloor; }
        float getCurrentEnvelope() const noexcept { return fastEnvelope.getEnvelope(); }

    private:
        double currentSampleRate{DSPConfig::DefaultSampleRate};
        
        // DSP processing elements
        BiquadFilter dcBlocker;
        BiquadFilter highPassFilter;
        EnvelopeFollower fastEnvelope;
        EnvelopeFollower slowEnvelope;

        // Parameters
        float thresholdNormalized{0.25f};
        float sensitivityFactor{2.0f};
        float minRetriggerSamples{1323.0f}; // ~30ms at 44.1k
        float noiseGateLinear{0.005f};

        // State tracking
        float noiseFloor{0.001f};
        float prevFastEnergy{0.0f};
        int   retriggerSampleCounter{0};
        float currentPeak{0.0f};
    };
}

