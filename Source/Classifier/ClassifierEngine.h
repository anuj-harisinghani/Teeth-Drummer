#pragma once

#include "ModelData.h"
#include "DistanceClassifier.h"
#include "RuleBasedFallback.h"
#include <memory>
#include <atomic>

namespace TeethDrummer
{
    class ClassifierEngine
    {
    public:
        ClassifierEngine();

        // Real-time safe classification method
        DrumPad classify(const FeatureVector& feats, float& outConfidence) const noexcept;

        // Profile access (GUI/calibration thread updates profile safely)
        void setProfile(const UserProfile& profile);
        const UserProfile& getProfile() const noexcept { return activeProfile; }

        void resetToDefaults();

    private:
        UserProfile activeProfile;
        DistanceClassifier distanceClassifier;
        bool hasAnyCalibratedPad{false};
    };
}

