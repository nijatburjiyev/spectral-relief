#include "DisplayRowProcessor.h"

#include <algorithm>
#include <cmath>

void DisplayRowProcessor::reset() noexcept
{
    previous = {};
    hasPrevious = false;
}

void DisplayRowProcessor::setSmoothing (float newSmoothing) noexcept
{
    smoothing = std::clamp (newSmoothing, 0.0f, 1.0f);
}

double DisplayRowProcessor::getTimeConstantSeconds() const noexcept
{
    return 1.5 * static_cast<double> (smoothing) * static_cast<double> (smoothing);
}

SpectrumRow DisplayRowProcessor::process (const SpectrumRow& input) noexcept
{
    SpectrumFrame frame;
    frame.bandCount = static_cast<std::uint16_t> (input.size());
    std::copy (input.begin(), input.end(), frame.magnitudes.begin());
    const auto processed = process (frame, 1.0 / 60.0);
    SpectrumRow output;
    std::copy_n (processed.magnitudes.begin(), output.size(), output.begin());
    return output;
}

SpectrumFrame DisplayRowProcessor::process (const SpectrumFrame& input) noexcept
{
    return process (input, 1.0 / 60.0);
}

SpectrumFrame DisplayRowProcessor::process (const SpectrumFrame& input,
                                            double elapsedSeconds) noexcept
{
    if (previous.bandCount != input.bandCount)
        reset();

    if (! hasPrevious || smoothing == 0.0f)
    {
        previous = input;
        hasPrevious = true;
        return input;
    }

    const auto timeConstant = getTimeConstantSeconds();
    const auto alpha = static_cast<float> (
        1.0 - std::exp (-std::max (0.0, elapsedSeconds) / timeConstant));
    SpectrumFrame output;
    output.bandCount = input.bandCount;
    for (std::size_t band = 0; band < input.bandCount; ++band)
        output.magnitudes[band] = previous.magnitudes[band]
                                + alpha * (input.magnitudes[band] - previous.magnitudes[band]);

    previous = output;
    return output;
}

void HistoryCadence::setDuration (double seconds) noexcept
{
    durationSeconds = std::clamp (seconds, 2.0, 8.0);
}

int HistoryCadence::rowsDue (double elapsedSeconds) noexcept
{
    const auto newRows = std::max (0.0, elapsedSeconds) * 256.0 / durationSeconds;
    accumulatedRows = std::min (8.0, accumulatedRows + newRows);
    const auto due = std::min (8, static_cast<int> (std::floor (accumulatedRows + 1.0e-9)));
    accumulatedRows -= due;
    return due;
}
