#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace TeethDrummer
{
    // High-performance direct form II transposed Biquad filter
    class BiquadFilter
    {
    public:
        enum class Type
        {
            LowPass,
            HighPass,
            BandPass
        };

        BiquadFilter() = default;

        void configure(Type filterType, float cutoffHz, float q, float sampleRate) noexcept
        {
            if (sampleRate <= 0.0f || cutoffHz <= 0.0f) return;
            
            // Clamp cutoff to valid Nyquist range
            const float nyquist = sampleRate * 0.495f;
            const float fc = std::clamp(cutoffHz, 10.0f, nyquist);
            const float omega = 2.0f * static_cast<float>(std::numbers::pi) * (fc / sampleRate);
            const float sn = std::sin(omega);
            const float cs = std::cos(omega);
            const float alpha = sn / (2.0f * std::max(0.01f, q));

            float a0 = 1.0f;

            switch (filterType)
            {
                case Type::LowPass:
                    b0 = (1.0f - cs) * 0.5f;
                    b1 = 1.0f - cs;
                    b2 = (1.0f - cs) * 0.5f;
                    a0 = 1.0f + alpha;
                    a1 = -2.0f * cs;
                    a2 = 1.0f - alpha;
                    break;

                case Type::HighPass:
                    b0 = (1.0f + cs) * 0.5f;
                    b1 = -(1.0f + cs);
                    b2 = (1.0f + cs) * 0.5f;
                    a0 = 1.0f + alpha;
                    a1 = -2.0f * cs;
                    a2 = 1.0f - alpha;
                    break;

                case Type::BandPass:
                    b0 = alpha;
                    b1 = 0.0f;
                    b2 = -alpha;
                    a0 = 1.0f + alpha;
                    a1 = -2.0f * cs;
                    a2 = 1.0f - alpha;
                    break;
            }

            // Normalize coefficients by a0
            const float invA0 = 1.0f / a0;
            b0 *= invA0;
            b1 *= invA0;
            b2 *= invA0;
            a1 *= invA0;
            a2 *= invA0;
        }

        inline float processSample(float in) noexcept
        {
            // Direct Form II Transposed structure
            const float out = b0 * in + z1;
            z1 = b1 * in - a1 * out + z2;
            z2 = b2 * in - a2 * out;
            return out;
        }

        void reset() noexcept
        {
            z1 = 0.0f;
            z2 = 0.0f;
        }

    private:
        float b0{1.0f}, b1{0.0f}, b2{0.0f};
        float a1{0.0f}, a2{0.0f};
        float z1{0.0f}, z2{0.0f};
    };
}

