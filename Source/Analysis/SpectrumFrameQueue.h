#pragma once

#include "SpectrumTypes.h"

#include <array>
#include <algorithm>
#include <atomic>
#include <cstdint>

class SpectrumFrameQueue
{
public:
    static constexpr std::uint32_t capacity = 8;
    static constexpr std::size_t payloadBytes = capacity * sizeof (SpectrumFrame);

    bool tryPush (const SpectrumFrame& frame) noexcept
    {
        const auto write = writeCounter.load (std::memory_order_relaxed);
        const auto read = readCounter.load (std::memory_order_acquire);

        if (write - read >= capacity)
            return false;

        frames[write % capacity] = frame;
        writeCounter.store (write + 1, std::memory_order_release);
        return true;
    }

    bool tryPop (SpectrumFrame& destination) noexcept
    {
        const auto read = readCounter.load (std::memory_order_relaxed);
        const auto write = writeCounter.load (std::memory_order_acquire);

        if (read == write)
            return false;

        destination = frames[read % capacity];
        readCounter.store (read + 1, std::memory_order_release);
        return true;
    }

    bool tryPush (const SpectrumRow& row) noexcept
    {
        SpectrumFrame frame;
        std::copy (row.begin(), row.end(), frame.magnitudes.begin());
        return tryPush (frame);
    }

    bool tryPop (SpectrumRow& destination) noexcept
    {
        SpectrumFrame frame;
        if (! tryPop (frame))
            return false;
        std::copy_n (frame.magnitudes.begin(), destination.size(), destination.begin());
        return true;
    }

    void requestDiscard() noexcept
    {
        discardRequested.store (true, std::memory_order_release);
    }

    void serviceDiscardRequest() noexcept
    {
        if (! discardRequested.exchange (false, std::memory_order_acq_rel))
            return;

        readCounter.store (writeCounter.load (std::memory_order_acquire),
                           std::memory_order_release);
    }

private:
    std::array<SpectrumFrame, capacity> frames {};
    std::atomic<std::uint32_t> writeCounter { 0 };
    std::atomic<std::uint32_t> readCounter { 0 };
    std::atomic<bool> discardRequested { false };
};
