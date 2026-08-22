#pragma once

#include "../Common/Constants.h"
#include "../DSP/FeatureExtractor.h"
#include <algorithm>

namespace TeethDrummer
{
    // Fast expert-heuristic classifier for immediate out-of-the-box operation
    class RuleBasedFallback
    {
    public:
        static DrumPad classify(const FeatureVector& f, float& outConfidence) noexcept
        {
            // 1. Check for Kick: Strong low-frequency acoustic energy
            if (f.lowEnergyRatio > 0.45f || (f.lowEnergyRatio > 0.35f && f.spectralCentroid < 1200.0f))
            {
                outConfidence = std::min(1.0f, f.lowEnergyRatio + 0.2f);
                return DrumPad::Kick;
            }

            // 2. Check for Hi-Hats: High frequency dominance
            if (f.highEnergyRatio > 0.45f || f.spectralCentroid > 4500.0f)
            {
                // Disambiguate Closed vs Open Hat using decay slope
                if (f.decaySlope > 0.50f)
                {
                    outConfidence = std::min(1.0f, f.highEnergyRatio * 0.8f + f.decaySlope * 0.3f);
                    return DrumPad::OpenHat;
                }
                else
                {
                    outConfidence = std::min(1.0f, f.highEnergyRatio * 0.9f + (1.0f - f.decaySlope) * 0.2f);
                    return DrumPad::ClosedHat;
                }
            }

            // 3. Check for Snare: Dominant mid-frequency resonance
            if (f.midEnergyRatio > 0.35f || (f.spectralCentroid >= 1200.0f && f.spectralCentroid <= 4500.0f))
            {
                outConfidence = std::min(1.0f, f.midEnergyRatio + 0.25f);
                return DrumPad::Snare;
            }

            // Fallback default based on highest band ratio
            if (f.lowEnergyRatio >= f.midEnergyRatio && f.lowEnergyRatio >= f.highEnergyRatio)
            {
                outConfidence = 0.5f;
                return DrumPad::Kick;
            }
            else if (f.midEnergyRatio >= f.lowEnergyRatio && f.midEnergyRatio >= f.highEnergyRatio)
            {
                outConfidence = 0.5f;
                return DrumPad::Snare;
            }
            else
            {
                outConfidence = 0.5f;
                return DrumPad::ClosedHat;
            }
        }
    };
}

