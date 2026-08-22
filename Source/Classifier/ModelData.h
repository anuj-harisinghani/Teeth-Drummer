#pragma once

#include <array>
#include <string>
#include "../Common/Constants.h"
#include "../DSP/FeatureExtractor.h"

namespace TeethDrummer
{
    // Acoustic prototype centroid for a single drum pad
    struct PadPrototype
    {
        DrumPad pad{DrumPad::None};
        bool isCalibrated{false};
        int sampleCount{0};

        // Normalized feature centroid
        float lowEnergyRatio{0.0f};
        float midEnergyRatio{0.0f};
        float highEnergyRatio{0.0f};
        float spectralCentroid{0.0f};
        float zeroCrossingRate{0.0f};
        float decaySlope{0.0f};

        // Variance / spread for Mahalanobis-like weighting
        float lowVariance{0.05f};
        float midVariance{0.05f};
        float highVariance{0.05f};
        float centroidVariance{500.0f};
    };

    // User acoustic calibration profile
    struct UserProfile
    {
        std::string profileName{"Default Profile"};
        std::array<PadPrototype, static_cast<size_t>(DrumPad::Count)> prototypes{};

        UserProfile()
        {
            initDefaults();
        }

        void initDefaults()
        {
            // Default Kick prototype (molars / closed cavity)
            prototypes[static_cast<size_t>(DrumPad::Kick)].pad = DrumPad::Kick;
            prototypes[static_cast<size_t>(DrumPad::Kick)].lowEnergyRatio = 0.65f;
            prototypes[static_cast<size_t>(DrumPad::Kick)].midEnergyRatio = 0.25f;
            prototypes[static_cast<size_t>(DrumPad::Kick)].highEnergyRatio = 0.10f;
            prototypes[static_cast<size_t>(DrumPad::Kick)].spectralCentroid = 600.0f;
            prototypes[static_cast<size_t>(DrumPad::Kick)].zeroCrossingRate = 0.08f;
            prototypes[static_cast<size_t>(DrumPad::Kick)].decaySlope = 0.3f;
            prototypes[static_cast<size_t>(DrumPad::Kick)].isCalibrated = false;

            // Default Snare prototype (mouth snap / mid resonance)
            prototypes[static_cast<size_t>(DrumPad::Snare)].pad = DrumPad::Snare;
            prototypes[static_cast<size_t>(DrumPad::Snare)].lowEnergyRatio = 0.15f;
            prototypes[static_cast<size_t>(DrumPad::Snare)].midEnergyRatio = 0.60f;
            prototypes[static_cast<size_t>(DrumPad::Snare)].highEnergyRatio = 0.25f;
            prototypes[static_cast<size_t>(DrumPad::Snare)].spectralCentroid = 2200.0f;
            prototypes[static_cast<size_t>(DrumPad::Snare)].zeroCrossingRate = 0.22f;
            prototypes[static_cast<size_t>(DrumPad::Snare)].decaySlope = 0.45f;
            prototypes[static_cast<size_t>(DrumPad::Snare)].isCalibrated = false;

            // Default Closed Hi-Hat prototype (front teeth sharp click)
            prototypes[static_cast<size_t>(DrumPad::ClosedHat)].pad = DrumPad::ClosedHat;
            prototypes[static_cast<size_t>(DrumPad::ClosedHat)].lowEnergyRatio = 0.05f;
            prototypes[static_cast<size_t>(DrumPad::ClosedHat)].midEnergyRatio = 0.20f;
            prototypes[static_cast<size_t>(DrumPad::ClosedHat)].highEnergyRatio = 0.75f;
            prototypes[static_cast<size_t>(DrumPad::ClosedHat)].spectralCentroid = 6500.0f;
            prototypes[static_cast<size_t>(DrumPad::ClosedHat)].zeroCrossingRate = 0.45f;
            prototypes[static_cast<size_t>(DrumPad::ClosedHat)].decaySlope = 0.15f; // Fast decay
            prototypes[static_cast<size_t>(DrumPad::ClosedHat)].isCalibrated = false;

            // Default Open Hi-Hat prototype (front click + sustained release)
            prototypes[static_cast<size_t>(DrumPad::OpenHat)].pad = DrumPad::OpenHat;
            prototypes[static_cast<size_t>(DrumPad::OpenHat)].lowEnergyRatio = 0.05f;
            prototypes[static_cast<size_t>(DrumPad::OpenHat)].midEnergyRatio = 0.30f;
            prototypes[static_cast<size_t>(DrumPad::OpenHat)].highEnergyRatio = 0.65f;
            prototypes[static_cast<size_t>(DrumPad::OpenHat)].spectralCentroid = 5500.0f;
            prototypes[static_cast<size_t>(DrumPad::OpenHat)].zeroCrossingRate = 0.40f;
            prototypes[static_cast<size_t>(DrumPad::OpenHat)].decaySlope = 0.75f; // Sustained tail
            prototypes[static_cast<size_t>(DrumPad::OpenHat)].isCalibrated = false;
        }
    };
}

