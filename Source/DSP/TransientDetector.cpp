#include "TransientDetector.h"
#include <cmath>

namespace TeethDrummer
{
    TransientDetector::TransientDetector()
    {
        prepare(DSPConfig::DefaultSampleRate, DSPConfig::MaxBlockSize);
    }

    void TransientDetector::prepare(double sampleRate, int /*maxBlockSize*/)
    {
        currentSampleRate = (sampleRate > 0.0) ? sampleRate : DSPConfig::DefaultSampleRate;

        // High-pass filter at 45Hz to eliminate sub-bass rumbles / mic handling
        highPassFilter.configure(BiquadFilter::Type::HighPass, 45.0f, 0.707f, static_cast<float>(currentSampleRate));
        
        // Fast envelope: attack 0.2ms, release 10ms
        fastEnvelope.configure(0.2f, 10.0f, static_cast<float>(currentSampleRate));

        // Slow envelope for noise floor tracking: attack 100ms, release 500ms
        slowEnvelope.configure(100.0f, 500.0f, static_cast<float>(currentSampleRate));

        setRetriggerTimeMs(DSPConfig::DefaultRetriggerMs);
        reset();
    }

    void TransientDetector::reset()
    {
        highPassFilter.reset();
        fastEnvelope.reset();
        slowEnvelope.reset();
        noiseFloor = 0.001f;
        prevFastEnergy = 0.0f;
        retriggerSampleCounter = 0;
        currentPeak = 0.0f;
    }

    void TransientDetector::setThreshold(float normalizedThreshold)
    {
        thresholdNormalized = std::clamp(normalizedThreshold, 0.01f, 1.0f);
    }

    void TransientDetector::setSensitivity(float sensitivity)
    {
        sensitivityFactor = std::clamp(sensitivity, 0.1f, 10.0f);
    }

    void TransientDetector::setRetriggerTimeMs(float ms)
    {
        const float clampedMs = std::clamp(ms, DSPConfig::MinRetriggerMs, DSPConfig::MaxRetriggerMs);
        minRetriggerSamples = static_cast<float>(clampedMs * 0.001f * currentSampleRate);
    }

    void TransientDetector::setNoiseGateFloor(float gateThreshold)
    {
        noiseGateLinear = std::max(0.0001f, gateThreshold);
    }

    bool TransientDetector::processSample(float inputSample, float& outPeakAmp)
    {
        // 1. High-pass filter
        const float filtered = highPassFilter.processSample(inputSample);

        // 2. Track fast and slow envelopes
        const float fastEnv = fastEnvelope.processSample(filtered);
        const float slowEnv = slowEnvelope.processSample(filtered);

        // Slowly update noise floor estimation
        noiseFloor = std::max(0.0005f, slowEnv);

        // 3. Compute energy derivative (slope of envelope)
        const float energyDiff = fastEnv - prevFastEnergy;
        prevFastEnergy = fastEnv;

        // Decrement retrigger counter
        if (retriggerSampleCounter > 0)
        {
            --retriggerSampleCounter;
            if (fastEnv > currentPeak)
                currentPeak = fastEnv;
            return false;
        }

        // 4. Threshold condition:
        // Must exceed absolute noise gate floor AND dynamic noise floor multiplier AND energy slope
        const float dynamicThreshold = noiseGateLinear + (thresholdNormalized * 0.2f) + (noiseFloor * (1.0f + sensitivityFactor * 0.5f));
        const float slopeThreshold = (thresholdNormalized * 0.05f) / sensitivityFactor;

        if (fastEnv > dynamicThreshold && energyDiff > slopeThreshold)
        {
            // Onset detected!
            outPeakAmp = std::max(fastEnv, std::abs(filtered));
            currentPeak = outPeakAmp;
            retriggerSampleCounter = static_cast<int>(minRetriggerSamples);
            return true;
        }

        return false;
    }
}

