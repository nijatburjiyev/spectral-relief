#pragma once

enum class AnalysisQuality
{
    normal,
    high,
    ultra
};

struct QualityProfile
{
    int fftOrder;
    int fftSize;
    int bandCount;
    int meshWidth;
    int meshDepth;
};

[[nodiscard]] inline constexpr QualityProfile getQualityProfile (AnalysisQuality quality) noexcept
{
    switch (quality)
    {
        case AnalysisQuality::ultra: return { 13, 8192, 1024, 1024, 256 };
        case AnalysisQuality::high:  return { 12, 4096, 512, 512, 256 };
        case AnalysisQuality::normal:return { 11, 2048, 256, 256, 256 };
    }

    return { 11, 2048, 256, 256, 256 };
}
