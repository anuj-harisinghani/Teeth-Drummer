#pragma once

#include "../Common/Constants.h"
#include "../Common/LockFreeQueue.h"
#include "../Classifier/ModelData.h"
#include "../Classifier/ClassifierEngine.h"
#include <vector>
#include <functional>
#include <atomic>

namespace TeethDrummer
{
    class CalibrationManager
    {
    public:
        CalibrationManager(ClassifierEngine& engine);

        // Start calibrating a specific pad (e.g. target 5 hits). GUI/message thread only.
        void startCalibration(DrumPad pad, int targetHits = 5);
        void cancelCalibration();

        bool isCalibrating() const noexcept { return calibratingPad.load(std::memory_order_relaxed) != DrumPad::None; }
        DrumPad getTargetPad() const noexcept { return calibratingPad.load(std::memory_order_relaxed); }
        int getCurrentHits() const noexcept { return static_cast<int>(recordedHits.size()); }
        int getTargetHits() const noexcept { return targetHitCount; }

        // Called on the AUDIO thread when a hit occurs. Lock-free and non-blocking:
        // just gates on an atomic and pushes into a lock-free queue, never touches
        // recordedHits/targetHitCount or takes any lock.
        bool queueCalibrationHit(const FeatureVector& feats) noexcept;

        // Called periodically on the GUI/message thread (e.g. from the editor's timer)
        // to drain hits queued by the audio thread and do the actual bookkeeping.
        void pumpCalibrationQueue();

        // Reset all or specific pad calibration. GUI/message thread only.
        void clearPadCalibration(DrumPad pad);
        void resetAll();

        // Listeners for UI updates
        using StateChangedCallback = std::function<void(DrumPad pad, int currentHits, int targetHits, bool finished)>;
        void setOnStateChanged(StateChangedCallback callback) { onStateChanged = std::move(callback); }

    private:
        void finalizeCalibration(DrumPad pad);

        ClassifierEngine& classifierEngine;

        // Written by the GUI thread, read lock-free by the audio thread as a simple gate.
        std::atomic<DrumPad> calibratingPad{DrumPad::None};

        // GUI/message-thread-only state - never touched by the audio thread directly.
        int targetHitCount{5};
        std::vector<FeatureVector> recordedHits;

        // Audio thread pushes, GUI/message thread (pumpCalibrationQueue) pops.
        LockFreeQueue<FeatureVector, 32> hitQueue;

        StateChangedCallback onStateChanged;
    };
}
