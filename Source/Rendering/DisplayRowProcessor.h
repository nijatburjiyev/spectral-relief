#pragma once

#include "Source/Analysis/SpectrumTypes.h"

class DisplayRowProcessor
{
public:
    void reset() noexcept;
    void setSmoothing (float newSmoothing) noexcept;
    float getSmoothing() const noexcept { return smoothing; }
    double getTimeConstantSeconds() const noexcept;
    SpectrumRow process (const SpectrumRow& input) noexcept;
    SpectrumFrame process (const SpectrumFrame& input) noexcept;
    SpectrumFrame process (const SpectrumFrame& input, double elapsedSeconds) noexcept;

private:
    SpectrumFrame previous {};
    float smoothing = 0.0f;
    bool hasPrevious = false;
};

class HistoryCadence
{
public:
    void reset() noexcept { accumulatedRows = 0.0; }
    void setDuration (double seconds) noexcept;
    int rowsDue (double elapsedSeconds) noexcept;

private:
    double durationSeconds = 5.0;
    double accumulatedRows = 0.0;
};
