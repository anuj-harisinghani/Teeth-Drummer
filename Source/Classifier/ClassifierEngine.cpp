#include "ClassifierEngine.h"

namespace TeethDrummer
{
    ClassifierEngine::ClassifierEngine()
    {
        resetToDefaults();
    }

    void ClassifierEngine::resetToDefaults()
    {
        const int idx = activeBufferIndex.load(std::memory_order_relaxed);
        const int nextIdx = 1 - idx;
        profileBuffers[nextIdx].initDefaults();
        activeBufferIndex.store(nextIdx, std::memory_order_release);
    }

    void ClassifierEngine::setProfile(const UserProfile& profile)
    {
        // Write into the currently-INACTIVE buffer, then publish it. The audio thread
        // (classify()) only ever reads through activeBufferIndex, so it never observes
        // a partially-written profile - no lock needed on either side.
        const int idx = activeBufferIndex.load(std::memory_order_relaxed);
        const int nextIdx = 1 - idx;
        profileBuffers[nextIdx] = profile;
        activeBufferIndex.store(nextIdx, std::memory_order_release);
    }

    UserProfile ClassifierEngine::getProfile() const noexcept
    {
        return profileBuffers[activeBufferIndex.load(std::memory_order_acquire)];
    }

    DrumPad ClassifierEngine::classify(const FeatureVector& feats, float& outConfidence) const noexcept
    {
        const UserProfile& profile = profileBuffers[activeBufferIndex.load(std::memory_order_acquire)];

        bool hasAnyCalibratedPad = false;
        for (const auto& proto : profile.prototypes)
        {
            if (proto.isCalibrated)
            {
                hasAnyCalibratedPad = true;
                break;
            }
        }

        // 1. If at least one pad has been explicitly calibrated by the user, use distance classification
        if (hasAnyCalibratedPad)
        {
            return distanceClassifier.classify(feats, profile, outConfidence);
        }

        // 2. Otherwise use factory default rule-based expert heuristic
        return RuleBasedFallback::classify(feats, outConfidence);
    }
}
