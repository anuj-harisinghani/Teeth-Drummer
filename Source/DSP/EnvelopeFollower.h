#pragma once

#include <cmath>
#include <algorithm>

namespace TeethDrummer
{
    // Fast dual-speed envelope follower for transient detection & noise gating
    class EnvelopeFollower
    {
    public:
        EnvelopeFollower() = default;

        void configure(float attackMs, float releaseMs, float sampleRate) noexcept
        {
            if (sampleRate <= 0.0f) return;
            
            attackCoeff  = std::exp(-1.0f / (std::max(0.01f, attackMs)  * 0.001f * sampleRate));
            releaseCoeff = std::exp(-1.0f / (std::max(0.01f, releaseMs) * 0.001f * sampleRate));
        }

        inline float processSample(float in) noexcept
        {
            const float absIn = std::abs(in);
            if (absIn > envelope)
                envelope = attackCoeff * envelope + (1.0f - attackCoeff) * absIn;
            else
                envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * absIn;

            return envelope;
        }

        inline float getEnvelope() const noexcept { return envelope; }

        void reset() noexcept { envelope = 0.0f; }

    private:
        float attackCoeff{0.0f};
        float releaseCoeff{0.0f};
        float envelope{0.0f};
    };
}

