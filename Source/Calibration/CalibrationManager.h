#pragma once

#include "../Common/Constants.h"
#include "../Classifier/ModelData.h"
#include "../Classifier/ClassifierEngine.h"
#include <vector>
#include <functional>
#include <mutex>

namespace TeethDrummer
{
    class CalibrationManager
    {
    public:
        CalibrationManager(ClassifierEngine& engine);

        // Start calibrating a specific pad (e.g. target 5 hits)
        void startCalibration(DrumPad pad, int targetHits = 5);
        void cancelCalibration();

        bool isCalibrating() const noexcept { return currentlyCalibratingPad != DrumPad::None; }
        DrumPad getTargetPad() const noexcept { return currentlyCalibratingPad; }
        int getCurrentHits() const noexcept { return static_cast<int>(recordedHits.size()); }
        int getTargetHits() const noexcept { return targetHitCount; }

        // Called on audio/telemetry thread when a hit occurs
        bool processCalibrationHit(const FeatureVector& feats);

        // Reset all or specific pad calibration
        void clearPadCalibration(DrumPad pad);
        void resetAll();

        // Listeners for UI updates
        using StateChangedCallback = std::function<void(DrumPad pad, int currentHits, int targetHits, bool finished)>;
        void setOnStateChanged(StateChangedCallback callback) { onStateChanged = std::move(callback); }

    private:
        void finalizeCalibration(DrumPad pad);

        ClassifierEngine& classifierEngine;
        DrumPad currentlyCalibratingPad{DrumPad::None};
        int targetHitCount{5};
        std::vector<FeatureVector> recordedHits;
        std::mutex calibrationMutex;

        StateChangedCallback onStateChanged;
    };
}

