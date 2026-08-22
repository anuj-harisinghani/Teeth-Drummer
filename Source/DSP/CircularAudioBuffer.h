#pragma once

#include <vector>
#include <array>
#include <cstddef>
#include <algorithm>
#include "../Common/Constants.h"

namespace TeethDrummer
{
    // Real-time safe static circular audio buffer
    class CircularAudioBuffer
    {
    public:
        CircularAudioBuffer() = default;

        inline void pushSample(float sample) noexcept
        {
            buffer[writeIndex] = sample;
            writeIndex = (writeIndex + 1) % DSPConfig::HistoryBufferSize;
        }

        // Copies the most recent `count` samples into `destination`
        void copyRecentSamples(float* destination, size_t count) const noexcept
        {
            if (destination == nullptr || count == 0) return;
            const size_t countClamped = std::min(count, static_cast<size_t>(DSPConfig::HistoryBufferSize));
            
            // Calculate start index in circular buffer
            const size_t startIdx = (writeIndex + DSPConfig::HistoryBufferSize - countClamped) % DSPConfig::HistoryBufferSize;

            for (size_t i = 0; i < countClamped; ++i)
            {
                destination[i] = buffer[(startIdx + i) % DSPConfig::HistoryBufferSize];
            }
        }

        void reset() noexcept
        {
            buffer.fill(0.0f);
            writeIndex = 0;
        }

    private:
        std::array<float, DSPConfig::HistoryBufferSize> buffer{};
        size_t writeIndex{0};
    };
}

