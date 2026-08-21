#pragma once

#include "Source/Analysis/SpectrumTypes.h"

#include <algorithm>
#include <cstdint>

class SpectrumFrameAggregator
{
public:
    void reset (std::uint16_t newBandCount = 0) noexcept
    {
        aggregate = {};
        activeBandCount = newBandCount;
        aggregate.bandCount = newBandCount;
        hasAggregate = false;
    }

    void push (const SpectrumFrame& frame) noexcept
    {
        if (activeBandCount == 0)
        {
            activeBandCount = frame.bandCount;
            aggregate.bandCount = frame.bandCount;
        }
        if (frame.bandCount != activeBandCount)
            return;

        for (std::size_t band = 0; band < activeBandCount; ++band)
            aggregate.magnitudes[band] = std::max (aggregate.magnitudes[band],
                                                   frame.magnitudes[band]);
        hasAggregate = true;
    }

    bool takePeakFrame (SpectrumFrame& destination) noexcept
    {
        if (! hasAggregate)
            return false;

        destination = aggregate;
        aggregate.magnitudes.fill (0.0f);
        aggregate.bandCount = activeBandCount;
        hasAggregate = false;
        return true;
    }

private:
    SpectrumFrame aggregate;
    std::uint16_t activeBandCount = 0;
    bool hasAggregate = false;
};
