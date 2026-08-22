#pragma once

#include <array>
#include <cstddef>
#include "../Common/Constants.h"
#include "BiquadFilter.h"

namespace TeethDrummer
{
    // Feature vector representing the acoustic fingerprint of a hit
    struct FeatureVector
    {
        float peakAmplitude{0.0f};      // Maximum absolute peak in window
        float rmsEnergy{0.0f};          // Root mean square energy
        float spectralCentroid{0.0f};   // Weighted center of mass of frequency spectrum (Hz)
        float zeroCrossingRate{0.0f};   // Fraction of sign changes per sample
        float lowEnergyRatio{0.0f};     // Ratio of energy in 60-350Hz
        float midEnergyRatio{0.0f};     // Ratio of energy in 350-3500Hz
        float highEnergyRatio{0.0f};    // Ratio of energy in 3500-12000Hz
        float crestFactor{0.0f};        // Peak / RMS ratio (impulsiveness)
        float decaySlope{0.0f};         // Ratio of energy in second half vs first half of window
    };

    class FeatureExtractor
    {
    public:
        FeatureExtractor();

        void prepare(double sampleRate);
        void reset();

        // Extracts feature vector from an audio slice (e.g. 128 samples / ~3ms)
        FeatureVector extract(const float* samples, size_t numSamples);

    private:
        double currentSampleRate{DSPConfig::DefaultSampleRate};

        // Filter bank for fast time-domain energy band separation
        BiquadFilter lowPassFilter;   // < 350 Hz
        BiquadFilter bandPassFilter;  // 350 - 3500 Hz
        BiquadFilter highPassFilter;  // > 3500 Hz
    };
}

