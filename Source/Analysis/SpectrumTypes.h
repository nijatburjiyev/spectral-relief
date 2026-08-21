#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

inline constexpr std::size_t spectrumBandCount = 256;
inline constexpr std::size_t maximumSpectrumBandCount = 1024;
using SpectrumRow = std::array<float, spectrumBandCount>;

struct SpectrumFrame
{
    std::array<float, maximumSpectrumBandCount> magnitudes {};
    std::uint16_t bandCount = static_cast<std::uint16_t> (spectrumBandCount);
};
