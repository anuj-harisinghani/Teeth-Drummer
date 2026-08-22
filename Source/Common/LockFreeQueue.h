#pragma once

#include <atomic>
#include <cstddef>
#include <vector>
#include <array>
#include "Constants.h"

namespace TeethDrummer
{
    // A single-producer single-consumer lock-free ring buffer for real-time safety
    template <typename T, size_t Capacity>
    class LockFreeQueue
    {
    public:
        LockFreeQueue() : readIdx(0), writeIdx(0) {}

        bool push(const T& item) noexcept
        {
            const size_t currentWrite = writeIdx.load(std::memory_order_relaxed);
            const size_t nextWrite = (currentWrite + 1) % Capacity;
            
            if (nextWrite == readIdx.load(std::memory_order_acquire))
                return false; // Queue is full

            buffer[currentWrite] = item;
            writeIdx.store(nextWrite, std::memory_order_release);
            return true;
        }

        bool pop(T& item) noexcept
        {
            const size_t currentRead = readIdx.load(std::memory_order_relaxed);
            
            if (currentRead == writeIdx.load(std::memory_order_acquire))
                return false; // Queue is empty

            item = buffer[currentRead];
            readIdx.store((currentRead + 1) % Capacity, std::memory_order_release);
            return true;
        }

        bool isEmpty() const noexcept
        {
            return readIdx.load(std::memory_order_acquire) == writeIdx.load(std::memory_order_acquire);
        }

        void reset() noexcept
        {
            readIdx.store(0, std::memory_order_release);
            writeIdx.store(0, std::memory_order_release);
        }

    private:
        std::array<T, Capacity> buffer{};
        alignas(64) std::atomic<size_t> writeIdx{0};
        alignas(64) std::atomic<size_t> readIdx{0};
    };

    // Telemetry event sent from Audio Thread to UI Thread upon hit detection
    struct TriggerTelemetryEvent
    {
        DrumPad pad{DrumPad::None};
        float velocity{0.0f};       // 0.0 to 1.0
        uint8_t midiVelocity{0};    // 1 to 127
        float peakMagnitude{0.0f};  // Raw peak dB or amplitude
        float spectralCentroid{0.0f};
        float lowEnergyRatio{0.0f};
        float midEnergyRatio{0.0f};
        float highEnergyRatio{0.0f};
        float confidence{0.0f};
        int64_t sampleTimestamp{0};
    };
}

