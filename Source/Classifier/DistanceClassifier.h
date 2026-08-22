#pragma once

#include "ModelData.h"
#include <array>

namespace TeethDrummer
{
    class DistanceClassifier
    {
    public:
        DistanceClassifier() = default;

        // Classifies a feature vector against the user profile.
        // Returns the winning DrumPad and calculates a confidence score (0.0 to 1.0)
        DrumPad classify(const FeatureVector& feats, const UserProfile& profile, float& outConfidence) const noexcept;

        // Computes raw normalized distance between a feature vector and a pad prototype
        float computeDistance(const FeatureVector& feats, const PadPrototype& proto) const noexcept;
    };
}

