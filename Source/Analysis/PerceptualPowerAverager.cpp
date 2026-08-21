#include "PerceptualPowerAverager.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr float floorDecibels = -100.0f;
constexpr float floorPower = 1.0e-10f;

float sanitiseDecibels (float decibels) noexcept
{
    return std::isfinite (decibels) ? std::max (decibels, floorDecibels)
                                    : floorDecibels;
}

float decibelsToPower (float decibels) noexcept
{
    const auto power = std::pow (10.0, static_cast<double> (decibels) / 10.0);
    if (! std::isfinite (power))
        return std::numeric_limits<float>::max();
    return static_cast<float> (std::clamp (
        power, static_cast<double> (floorPower),
        static_cast<double> (std::numeric_limits<float>::max())));
}
} // namespace

void PerceptualPowerAverager::setAverageMilliseconds (float newMilliseconds) noexcept
{
    const auto safeMilliseconds = std::isfinite (newMilliseconds)
        ? std::clamp (newMilliseconds, 0.0f, 1000.0f)
        : 0.0f;

    averageMilliseconds = safeMilliseconds;
    if (averageMilliseconds == 0.0f)
        reset();
}

void PerceptualPowerAverager::reset() noexcept
{
    previousPower.fill (floorPower);
    initializedBandCount = 0;
    hasPrevious = false;
}

double PerceptualPowerAverager::getAttackSeconds() const noexcept
{
    return attackSecondsForMilliseconds (averageMilliseconds);
}

double PerceptualPowerAverager::getReleaseSeconds() const noexcept
{
    return releaseSecondsForMilliseconds (averageMilliseconds);
}

double PerceptualPowerAverager::attackSecondsForMilliseconds (float milliseconds) noexcept
{
    if (! std::isfinite (milliseconds) || milliseconds <= 0.0f)
        return 0.0;
    return std::max (0.005, static_cast<double> (std::min (milliseconds, 1000.0f)) * 0.00025);
}

double PerceptualPowerAverager::releaseSecondsForMilliseconds (float milliseconds) noexcept
{
    if (! std::isfinite (milliseconds) || milliseconds <= 0.0f)
        return 0.0;
    return std::max (0.005, static_cast<double> (std::min (milliseconds, 1000.0f)) * 0.001);
}

void PerceptualPowerAverager::processDecibels (std::span<float> decibels,
                                               double elapsedSeconds) noexcept
{
    if (averageMilliseconds == 0.0f)
        return;

    const auto count = std::min (decibels.size(), previousPower.size());
    if (count == 0)
    {
        reset();
        return;
    }

    if (! hasPrevious || initializedBandCount != count)
    {
        for (std::size_t band = 0; band < count; ++band)
        {
            decibels[band] = sanitiseDecibels (decibels[band]);
            previousPower[band] = decibelsToPower (decibels[band]);
        }
        initializedBandCount = count;
        hasPrevious = true;
        return;
    }

    const auto safeElapsed = std::isfinite (elapsedSeconds)
        ? std::max (0.0, elapsedSeconds)
        : 0.0;
    const auto attackAlpha = static_cast<float> (
        1.0 - std::exp (-safeElapsed / getAttackSeconds()));
    const auto releaseAlpha = static_cast<float> (
        1.0 - std::exp (-safeElapsed / getReleaseSeconds()));

    for (std::size_t band = 0; band < count; ++band)
    {
        const auto inputPower = decibelsToPower (sanitiseDecibels (decibels[band]));
        const auto alpha = inputPower >= previousPower[band] ? attackAlpha : releaseAlpha;
        previousPower[band] += alpha * (inputPower - previousPower[band]);
        decibels[band] = 10.0f * std::log10 (std::max (previousPower[band], floorPower));
    }
}
