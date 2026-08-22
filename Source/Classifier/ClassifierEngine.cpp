#include "ClassifierEngine.h"

namespace TeethDrummer
{
    ClassifierEngine::ClassifierEngine()
    {
        resetToDefaults();
    }

    void ClassifierEngine::resetToDefaults()
    {
        activeProfile.initDefaults();
        hasAnyCalibratedPad = false;
    }

    void ClassifierEngine::setProfile(const UserProfile& profile)
    {
        activeProfile = profile;
        hasAnyCalibratedPad = false;
        for (const auto& proto : activeProfile.prototypes)
        {
            if (proto.isCalibrated)
            {
                hasAnyCalibratedPad = true;
                break;
            }
        }
    }

    DrumPad ClassifierEngine::classify(const FeatureVector& feats, float& outConfidence) const noexcept
    {
        // 1. If at least one pad has been explicitly calibrated by the user, use distance classification
        if (hasAnyCalibratedPad)
        {
            return distanceClassifier.classify(feats, activeProfile, outConfidence);
        }

        // 2. Otherwise use factory default rule-based expert heuristic
        return RuleBasedFallback::classify(feats, outConfidence);
    }
}

