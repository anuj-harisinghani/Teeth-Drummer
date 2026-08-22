#include "FeatureExtractor.h"
#include <cmath>
#include <algorithm>

namespace TeethDrummer
{
    FeatureExtractor::FeatureExtractor()
    {
        prepare(DSPConfig::DefaultSampleRate);
    }

    void FeatureExtractor::prepare(double sampleRate)
    {
        currentSampleRate = (sampleRate > 0.0) ? sampleRate : DSPConfig::DefaultSampleRate;
        const float sr = static_cast<float>(currentSampleRate);

        // Low band filter: Low-pass at 350 Hz
        lowPassFilter.configure(BiquadFilter::Type::LowPass, DSPConfig::LowBandCutoff, 0.707f, sr);

        // Mid band filter: Band-pass centered at ~1600 Hz (Q=0.8)
        bandPassFilter.configure(BiquadFilter::Type::BandPass, 1600.0f, 0.8f, sr);

        // High band filter: High-pass at 3500 Hz
        highPassFilter.configure(BiquadFilter::Type::HighPass, DSPConfig::HighBandCutoff, 0.707f, sr);

        reset();
    }

    void FeatureExtractor::reset()
    {
        lowPassFilter.reset();
        bandPassFilter.reset();
        highPassFilter.reset();
    }

    FeatureVector FeatureExtractor::extract(const float* samples, size_t numSamples)
    {
        FeatureVector feats{};
        if (samples == nullptr || numSamples == 0)
            return feats;

        float sumSquares = 0.0f;
        float peak = 0.0f;
        int zeroCrossings = 0;
        float prevSample = samples[0];

        float lowSumSq = 0.0f;
        float midSumSq = 0.0f;
        float highSumSq = 0.0f;

        const size_t halfPoint = numSamples / 2;
        float firstHalfEnergy = 0.0f;
        float secondHalfEnergy = 0.0f;

        // Reset filter states for isolated transient window processing
        reset();

        for (size_t i = 0; i < numSamples; ++i)
        {
            const float s = samples[i];
            const float absS = std::abs(s);

            if (absS > peak)
                peak = absS;

            const float sSq = s * s;
            sumSquares += sSq;

            if (i < halfPoint)
                firstHalfEnergy += sSq;
            else
                secondHalfEnergy += sSq;

            // Zero-crossing detection
            if ((s >= 0.0f && prevSample < 0.0f) || (s < 0.0f && prevSample >= 0.0f))
                ++zeroCrossings;
            prevSample = s;

            // Filter bank separation
            const float lowOut = lowPassFilter.processSample(s);
            const float midOut = bandPassFilter.processSample(s);
            const float highOut = highPassFilter.processSample(s);

            lowSumSq += lowOut * lowOut;
            midSumSq += midOut * midOut;
            highSumSq += highOut * highOut;
        }

        feats.peakAmplitude = peak;
        feats.rmsEnergy = std::sqrt(sumSquares / static_cast<float>(numSamples));
        feats.zeroCrossingRate = static_cast<float>(zeroCrossings) / static_cast<float>(numSamples);

        const float totalBandEnergy = lowSumSq + midSumSq + highSumSq + 1e-9f;
        feats.lowEnergyRatio = lowSumSq / totalBandEnergy;
        feats.midEnergyRatio = midSumSq / totalBandEnergy;
        feats.highEnergyRatio = highSumSq / totalBandEnergy;

        // Crest factor (Peak / RMS)
        feats.crestFactor = (feats.rmsEnergy > 1e-6f) ? (peak / feats.rmsEnergy) : 1.0f;

        // Decay slope
        feats.decaySlope = (firstHalfEnergy > 1e-9f) ? (secondHalfEnergy / firstHalfEnergy) : 0.0f;

        // Estimated Spectral Centroid from band weighting and ZCR
        // Center frequencies: Low ~200Hz, Mid ~1800Hz, High ~7000Hz
        const float bandCentroid = (feats.lowEnergyRatio * 200.0f) +
                                   (feats.midEnergyRatio * 1800.0f) +
                                   (feats.highEnergyRatio * 7000.0f);
        const float zcrCentroid = feats.zeroCrossingRate * static_cast<float>(currentSampleRate * 0.5f);
        
        feats.spectralCentroid = (bandCentroid * 0.6f) + (zcrCentroid * 0.4f);

        return feats;
    }
}

