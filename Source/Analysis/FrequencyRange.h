#pragma once

enum class FrequencyRange
{
    full,
    low,
    mid,
    high
};

struct FrequencyRangeProfile
{
    double minimumHz;
    double maximumHz;
};

[[nodiscard]] inline constexpr FrequencyRangeProfile getFrequencyRangeProfile (FrequencyRange range) noexcept
{
    switch (range)
    {
        case FrequencyRange::low:  return { 20.0, 500.0 };
        case FrequencyRange::mid:  return { 200.0, 5000.0 };
        case FrequencyRange::high: return { 2000.0, 20000.0 };
        case FrequencyRange::full: return { 20.0, 20000.0 };
    }

    return { 20.0, 20000.0 };
}
