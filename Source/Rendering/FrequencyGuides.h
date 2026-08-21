#pragma once

#include "Source/Analysis/FrequencyRange.h"

#include <array>
#include <cstddef>

struct FrequencyGuide
{
    double frequencyHz = 0.0;
    float normalisedFrequency = 0.0f;
};

struct FrequencyGuideSet
{
    std::array<FrequencyGuide, 10> values {};
    std::size_t count = 0;
};

[[nodiscard]] FrequencyGuideSet makeFrequencyGuides (FrequencyRangeProfile,
                                                     double nyquist) noexcept;
