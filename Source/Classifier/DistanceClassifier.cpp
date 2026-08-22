#include "DistanceClassifier.h"
#include <cmath>
#include <limits>
#include <algorithm>

namespace TeethDrummer
{
    float DistanceClassifier::computeDistance(const FeatureVector& f, const PadPrototype& p) const noexcept
    {
        // Feature weights
        constexpr float wLow   = 3.0f;
        constexpr float wMid   = 2.5f;
        constexpr float wHigh  = 3.0f;
        constexpr float wCent  = 1.5f;
        constexpr float wZcr   = 1.0f;
        constexpr float wDecay = 2.0f;

        const float dLow   = f.lowEnergyRatio - p.lowEnergyRatio;
        const float dMid   = f.midEnergyRatio - p.midEnergyRatio;
        const float dHigh  = f.highEnergyRatio - p.highEnergyRatio;
        const float dCent  = (f.spectralCentroid - p.spectralCentroid) / 3000.0f;
        const float dZcr   = f.zeroCrossingRate - p.zeroCrossingRate;
        const float dDecay = f.decaySlope - p.decaySlope;

        const float distSq = (wLow * dLow * dLow) +
                             (wMid * dMid * dMid) +
                             (wHigh * dHigh * dHigh) +
                             (wCent * dCent * dCent) +
                             (wZcr * dZcr * dZcr) +
                             (wDecay * dDecay * dDecay);

        return std::sqrt(std::max(0.0f, distSq));
    }

    DrumPad DistanceClassifier::classify(const FeatureVector& feats, const UserProfile& profile, float& outConfidence) const noexcept
    {
        float minDist = std::numeric_limits<float>::max();
        float secondMinDist = std::numeric_limits<float>::max();
        DrumPad bestPad = DrumPad::None;

        std::array<float, static_cast<size_t>(DrumPad::Count)> distances{};

        for (size_t i = 0; i < static_cast<size_t>(DrumPad::Count); ++i)
        {
            const auto& proto = profile.prototypes[i];
            const float dist = computeDistance(feats, proto);
            distances[i] = dist;

            if (dist < minDist)
            {
                secondMinDist = minDist;
                minDist = dist;
                bestPad = proto.pad;
            }
            else if (dist < secondMinDist)
            {
                secondMinDist = dist;
            }
        }

        // Calculate confidence ratio based on separation between 1st and 2nd closest
        if (minDist < 1e-4f)
        {
            outConfidence = 1.0f;
        }
        else
        {
            const float margin = secondMinDist - minDist;
            outConfidence = std::clamp(0.5f + (margin / (minDist + 0.5f)) * 0.5f, 0.1f, 1.0f);
        }

        return bestPad;
    }
}

