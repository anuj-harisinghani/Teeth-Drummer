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
        std::lock_guard<std::mutex> lock(calibrationMutex);
        currentlyCalibratingPad = pad;
        targetHitCount = std::clamp(targetHits, 3, 20);
        recordedHits.clear();
        recordedHits.reserve(targetHitCount);

        if (onStateChanged)
            onStateChanged(currentlyCalibratingPad, 0, targetHitCount, false);
    }

    void CalibrationManager::cancelCalibration()
    {
        std::lock_guard<std::mutex> lock(calibrationMutex);
        const auto prevPad = currentlyCalibratingPad;
        currentlyCalibratingPad = DrumPad::None;
        recordedHits.clear();

        if (onStateChanged)
            onStateChanged(prevPad, 0, targetHitCount, false);
    }

    bool CalibrationManager::processCalibrationHit(const FeatureVector& feats)
    {
        std::lock_guard<std::mutex> lock(calibrationMutex);
        if (currentlyCalibratingPad == DrumPad::None)
            return false;

        recordedHits.push_back(feats);
        const int currentHits = static_cast<int>(recordedHits.size());
        const bool isFinished = (currentHits >= targetHitCount);

        if (onStateChanged)
            onStateChanged(currentlyCalibratingPad, currentHits, targetHitCount, isFinished);

        if (isFinished)
        {
            finalizeCalibration(currentlyCalibratingPad);
            currentlyCalibratingPad = DrumPad::None;
        }

        return true;
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
        std::lock_guard<std::mutex> lock(calibrationMutex);
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
        std::lock_guard<std::mutex> lock(calibrationMutex);
        classifierEngine.resetToDefaults();
        recordedHits.clear();
        currentlyCalibratingPad = DrumPad::None;

        if (onStateChanged)
            onStateChanged(DrumPad::None, 0, targetHitCount, false);
    }
}

