#include "FrequencyGuides.h"

#include <algorithm>
#include <array>
#include <cmath>

FrequencyGuideSet makeFrequencyGuides (FrequencyRangeProfile range, double nyquist) noexcept
{
    constexpr std::array<double, 10> major {
        20.0, 50.0, 100.0, 200.0, 500.0,
        1000.0, 2000.0, 5000.0, 10000.0, 20000.0
    };
    FrequencyGuideSet result;
    const auto maximum = std::min (range.maximumHz, std::max (range.minimumHz, nyquist));
    const auto logSpan = std::log (range.maximumHz / range.minimumHz);
    if (! std::isfinite (logSpan) || logSpan <= 0.0)
        return result;

    for (const auto frequency : major)
    {
        if (frequency < range.minimumHz || frequency > maximum)
            continue;
        const auto proportion = std::log (frequency / range.minimumHz) / logSpan;
        result.values[result.count++] = {
            frequency,
            static_cast<float> (2.0 * proportion - 1.0)
        };
    }
    return result;
}
