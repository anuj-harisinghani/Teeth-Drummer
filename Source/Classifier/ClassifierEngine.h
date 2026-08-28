#pragma once

#include "ModelData.h"
#include "DistanceClassifier.h"
#include "RuleBasedFallback.h"
#include <memory>
#include <array>
#include <atomic>

namespace TeethDrummer
{
    class ClassifierEngine
    {
    public:
        ClassifierEngine();

        // Real-time safe classification method (audio thread)
        DrumPad classify(const FeatureVector& feats, float& outConfidence) const noexcept;

        // Profile access (GUI/calibration thread updates profile safely).
        // Internally double-buffered so the audio thread's classify() never reads a
        // profile that is being concurrently written by the GUI/calibration thread.
        void setProfile(const UserProfile& profile);
        UserProfile getProfile() const noexcept;

        void resetToDefaults();

    private:
        std::array<UserProfile, 2> profileBuffers;
        std::atomic<int> activeBufferIndex{0};
        DistanceClassifier distanceClassifier;
    };
}
