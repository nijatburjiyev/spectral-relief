#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

class AudioSampleQueue
{
public:
    static constexpr std::size_t capacity = 65536;
    static constexpr std::size_t payloadBytes = capacity * sizeof (float);

    bool tryPush (float sample) noexcept
    {
        const auto write = writeCounter.load (std::memory_order_relaxed);
        const auto read = readCounter.load (std::memory_order_acquire);
        if (write - read >= capacity)
            return false;

        samples[static_cast<std::size_t> (write % capacity)] = sample;
        writeCounter.store (write + 1, std::memory_order_release);
        return true;
    }

    bool tryPop (float& sample) noexcept
    {
        std::uint64_t ignoredSequence = 0;
        return tryPop (sample, ignoredSequence);
    }

    bool tryPop (float& sample, std::uint64_t& sequence) noexcept
    {
        const auto read = readCounter.load (std::memory_order_relaxed);
        const auto write = writeCounter.load (std::memory_order_acquire);
        if (read == write)
            return false;

        sequence = read;
        sample = samples[static_cast<std::size_t> (read % capacity)];
        readCounter.store (read + 1, std::memory_order_release);
        return true;
    }

    void discard() noexcept
    {
        readCounter.store (writeCounter.load (std::memory_order_acquire),
                           std::memory_order_release);
    }

    [[nodiscard]] std::uint64_t getWriteSequence() const noexcept
    {
        return writeCounter.load (std::memory_order_acquire);
    }

    void discardBefore (std::uint64_t boundary) noexcept
    {
        const auto read = readCounter.load (std::memory_order_relaxed);
        const auto publishedWrite = writeCounter.load (std::memory_order_acquire);
        const auto target = std::min (boundary, publishedWrite);
        if (target > read)
            readCounter.store (target, std::memory_order_release);
    }

private:
    std::array<float, capacity> samples {};
    std::atomic<std::uint64_t> writeCounter { 0 };
    std::atomic<std::uint64_t> readCounter { 0 };
};
