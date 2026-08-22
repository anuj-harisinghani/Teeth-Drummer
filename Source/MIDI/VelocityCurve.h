#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>

namespace TeethDrummer
{
    class VelocityCurve
    {
    public:
        VelocityCurve() = default;

        void setSensitivity(float sens) noexcept { sensitivity = std::clamp(sens, 0.1f, 5.0f); }
        void setCurveExponent(float curve) noexcept { curveExp = std::clamp(curve, 0.2f, 3.0f); }
        void setMinVelocity(uint8_t minVel) noexcept { minVelocity = std::clamp<uint8_t>(minVel, 1, 127); }
        void setMaxVelocity(uint8_t maxVel) noexcept { maxVelocity = std::clamp<uint8_t>(maxVel, 1, 127); }

        // Maps raw peak amplitude (e.g. 0.005 to 1.0) to MIDI Velocity (1-127)
        uint8_t mapToVelocity(float peakAmp) const noexcept
        {
            if (peakAmp <= 0.0001f)
                return minVelocity;

            // Convert to normalized 0..1 range with sensitivity gain
            const float gained = peakAmp * sensitivity * 3.0f;
            const float clamped = std::clamp(gained, 0.0f, 1.0f);

            // Apply power curve: < 1.0 expands quiet hits (log-like), > 1.0 compresses quiet hits (exp-like)
            const float curved = std::pow(clamped, curveExp);

            const float minV = static_cast<float>(minVelocity);
            const float maxV = static_cast<float>(std::max(minVelocity, maxVelocity));

            const float mapped = minV + curved * (maxV - minV);
            return static_cast<uint8_t>(std::clamp(std::round(mapped), 1.0f, 127.0f));
        }

    private:
        float sensitivity{1.2f};
        float curveExp{0.8f}; // slightly logarithmic to enhance dynamics on micro-clicks
        uint8_t minVelocity{15};
        uint8_t maxVelocity{127};
    };
}

