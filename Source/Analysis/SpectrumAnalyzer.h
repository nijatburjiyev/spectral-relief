#pragma once

#include "PerceptualPowerAverager.h"
#include "SpectrumFrameQueue.h"
#include "Source/Diagnostics/RuntimeDiagnostics.h"

#include <juce_dsp/juce_dsp.h>

#include <span>
#include <vector>

class SpectrumAnalyzer
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = 256;

    explicit SpectrumAnalyzer (int configuredFftOrder = fftOrder,
                               int configuredBandCount = static_cast<int> (spectrumBandCount),
                               int configuredHopSize = hopSize);

    void prepare (double newSampleRate) noexcept;
    void setFrequencyRange (double minimumHz, double maximumHz) noexcept;
    void setAverageMilliseconds (float newMilliseconds) noexcept
    {
        powerAverager.setAverageMilliseconds (newMilliseconds);
    }
    void resetAveraging() noexcept { powerAverager.reset(); }
    void reset() noexcept;

    void process (const float* const* channels,
                  int numChannels,
                  int numSamples,
                  SpectrumFrameQueue& destination,
                  RuntimeDiagnostics& diagnostics) noexcept;
    void processMono (std::span<const float> samples,
                      SpectrumFrameQueue& destination,
                      RuntimeDiagnostics& diagnostics) noexcept;

    double getUpperFrequency() const noexcept { return upperFrequency; }
    double getLowerFrequency() const noexcept { return lowerFrequency; }
    int getConfiguredFftSize() const noexcept { return configuredFftSize; }
    int getBandCount() const noexcept { return configuredBandCount; }

private:
    void consumeSample (float, SpectrumFrameQueue&, RuntimeDiagnostics&) noexcept;
    void produceFrame (SpectrumFrameQueue&, RuntimeDiagnostics&) noexcept;
    float interpolateDecibels (double frequency) const noexcept;

    const int configuredFftOrder;
    const int configuredFftSize;
    const int configuredBandCount;
    const int configuredHopSize;
    juce::dsp::FFT fft;
    std::vector<float> inputRing;
    std::vector<float> fftData;
    std::vector<float> decibels;
    std::vector<float> bandDecibels;
    std::vector<float> window;
    std::vector<double> bandEdges;
    PerceptualPowerAverager powerAverager;

    double sampleRate = 48000.0;
    double lowerFrequency = 20.0;
    double upperFrequency = 20000.0;
    float windowCoherentGain = 0.5f;
    int writePosition = 0;
    int filledSamples = 0;
    int samplesSinceFrame = 0;
    bool producedFirstFrame = false;
};
