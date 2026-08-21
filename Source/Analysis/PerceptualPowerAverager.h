#pragma once

#include "SpectrumTypes.h"

#include <array>
#include <span>

class PerceptualPowerAverager
{
public:
    void setAverageMilliseconds (float newMilliseconds) noexcept;
    void reset() noexcept;
    void processDecibels (std::span<float> decibels, double elapsedSeconds) noexcept;

    [[nodiscard]] float getAverageMilliseconds() const noexcept { return averageMilliseconds; }
    [[nodiscard]] double getAttackSeconds() const noexcept;
    [[nodiscard]] double getReleaseSeconds() const noexcept;
    [[nodiscard]] static double attackSecondsForMilliseconds (float) noexcept;
    [[nodiscard]] static double releaseSecondsForMilliseconds (float) noexcept;

private:
    std::array<float, maximumSpectrumBandCount> previousPower {};
    std::size_t initializedBandCount = 0;
    float averageMilliseconds = 0.0f;
    bool hasPrevious = false;
};
