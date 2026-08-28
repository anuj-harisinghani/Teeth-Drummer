#include "CalibrationManager.h"
#include <numeric>
#include <algorithm>

namespace TeethDrummer
{
    CalibrationManager::CalibrationManager(ClassifierEngine& engine)
        : classifierEngine(engine)
    {
    }

    void CalibrationManager::startCalibration(DrumPad pad, int targetHits)
    {
        hitQueue.reset(); // discard any stale hits left over from a previous session
        recordedHits.clear();
        targetHitCount = std::clamp(targetHits, 3, 20);
        recordedHits.reserve(static_cast<size_t>(targetHitCount));
        calibratingPad.store(pad, std::memory_order_relaxed);

        if (onStateChanged)
            onStateChanged(pad, 0, targetHitCount, false);
    }

    void CalibrationManager::cancelCalibration()
    {
        const auto prevPad = calibratingPad.exchange(DrumPad::None, std::memory_order_relaxed);
        recordedHits.clear();
        hitQueue.reset();

        if (onStateChanged)
            onStateChanged(prevPad, 0, targetHitCount, false);
    }

    bool CalibrationManager::queueCalibrationHit(const FeatureVector& feats) noexcept
    {
        if (calibratingPad.load(std::memory_order_relaxed) == DrumPad::None)
            return false;

        return hitQueue.push(feats);
    }

    void CalibrationManager::pumpCalibrationQueue()
    {
        FeatureVector feats;
        while (hitQueue.pop(feats))
        {
            const DrumPad pad = calibratingPad.load(std::memory_order_relaxed);
            if (pad == DrumPad::None)
                continue; // calibration was cancelled after this hit was queued - discard it

            recordedHits.push_back(feats);
            const int currentHits = static_cast<int>(recordedHits.size());
            const bool isFinished = (currentHits >= targetHitCount);

            if (onStateChanged)
                onStateChanged(pad, currentHits, targetHitCount, isFinished);

            if (isFinished)
            {
                finalizeCalibration(pad);
                calibratingPad.store(DrumPad::None, std::memory_order_relaxed);
            }
        }
    }

    void CalibrationManager::finalizeCalibration(DrumPad pad)
    {
        if (recordedHits.empty() || pad == DrumPad::None)
            return;

        const size_t padIdx = static_cast<size_t>(pad);
        auto profile = classifierEngine.getProfile();
        auto& proto = profile.prototypes[padIdx];

        // Compute mean of features
        float sumLow = 0.0f, sumMid = 0.0f, sumHigh = 0.0f;
        float sumCent = 0.0f, sumZcr = 0.0f, sumDecay = 0.0f;

        for (const auto& h : recordedHits)
        {
            sumLow   += h.lowEnergyRatio;
            sumMid   += h.midEnergyRatio;
            sumHigh  += h.highEnergyRatio;
            sumCent  += h.spectralCentroid;
            sumZcr   += h.zeroCrossingRate;
            sumDecay += h.decaySlope;
        }

        const float count = static_cast<float>(recordedHits.size());
        proto.pad = pad;
        proto.isCalibrated = true;
        proto.sampleCount = static_cast<int>(recordedHits.size());
        proto.lowEnergyRatio   = sumLow / count;
        proto.midEnergyRatio   = sumMid / count;
        proto.highEnergyRatio  = sumHigh / count;
        proto.spectralCentroid = sumCent / count;
        proto.zeroCrossingRate = sumZcr / count;
        proto.decaySlope       = sumDecay / count;

        classifierEngine.setProfile(profile);
    }

    void CalibrationManager::clearPadCalibration(DrumPad pad)
    {
        if (pad == DrumPad::None) return;

        auto profile = classifierEngine.getProfile();
        UserProfile defaults;
        defaults.initDefaults();

        const size_t idx = static_cast<size_t>(pad);
        profile.prototypes[idx] = defaults.prototypes[idx];
        classifierEngine.setProfile(profile);

        if (onStateChanged)
            onStateChanged(pad, 0, targetHitCount, false);
    }

    void CalibrationManager::resetAll()
    {
        classifierEngine.resetToDefaults();
        recordedHits.clear();
        hitQueue.reset();
        calibratingPad.store(DrumPad::None, std::memory_order_relaxed);

        if (onStateChanged)
            onStateChanged(DrumPad::None, 0, targetHitCount, false);
    }
}
