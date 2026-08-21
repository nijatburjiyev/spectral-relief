#include "SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <numeric>

SpectrumAnalyzer::SpectrumAnalyzer (int newFftOrder, int newBandCount, int newHopSize)
    : configuredFftOrder (newFftOrder),
      configuredFftSize (1 << configuredFftOrder),
      configuredBandCount (juce::jlimit (1, static_cast<int> (maximumSpectrumBandCount), newBandCount)),
      configuredHopSize (juce::jmax (1, newHopSize)),
      fft (configuredFftOrder),
      inputRing (static_cast<std::size_t> (configuredFftSize)),
      fftData (static_cast<std::size_t> (configuredFftSize * 2)),
      decibels (static_cast<std::size_t> (configuredFftSize / 2 + 1)),
      bandDecibels (static_cast<std::size_t> (configuredBandCount), -100.0f),
      window (static_cast<std::size_t> (configuredFftSize)),
      bandEdges (static_cast<std::size_t> (configuredBandCount + 1))
{
}

void SpectrumAnalyzer::prepare (double newSampleRate) noexcept
{
    sampleRate = std::max (1.0, newSampleRate);
    juce::dsp::WindowingFunction<float>::fillWindowingTables (
        window.data(), window.size(), juce::dsp::WindowingFunction<float>::hann, false);
    windowCoherentGain = std::accumulate (window.begin(), window.end(), 0.0f)
                         / static_cast<float> (configuredFftSize);

    setFrequencyRange (20.0, 20000.0);
}

void SpectrumAnalyzer::setFrequencyRange (double minimumHz, double maximumHz) noexcept
{
    const auto nyquist = std::max (1.0, sampleRate * 0.5);
    lowerFrequency = std::clamp (minimumHz, 1.0, std::max (1.0, nyquist - 1.0));
    upperFrequency = std::clamp (maximumHz, lowerFrequency + 1.0, nyquist);
    const auto ratio = upperFrequency / lowerFrequency;
    for (std::size_t band = 0; band < bandEdges.size(); ++band)
    {
        const auto proportion = static_cast<double> (band) / configuredBandCount;
        bandEdges[band] = lowerFrequency * std::pow (ratio, proportion);
    }

    reset();
}

void SpectrumAnalyzer::reset() noexcept
{
    std::fill (inputRing.begin(), inputRing.end(), 0.0f);
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::fill (decibels.begin(), decibels.end(), -100.0f);
    std::fill (bandDecibels.begin(), bandDecibels.end(), -100.0f);
    powerAverager.reset();
    writePosition = 0;
    filledSamples = 0;
    samplesSinceFrame = 0;
    producedFirstFrame = false;
}

void SpectrumAnalyzer::process (const float* const* channels,
                                int numChannels,
                                int numSamples,
                                SpectrumFrameQueue& destination,
                                RuntimeDiagnostics& diagnostics) noexcept
{
    if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mono = 0.0f;
        int validChannels = 0;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            if (channels[channel] == nullptr)
                continue;
            mono += channels[channel][sample];
            ++validChannels;
        }

        if (validChannels > 0)
            mono /= static_cast<float> (validChannels);

        consumeSample (mono, destination, diagnostics);
    }
}

void SpectrumAnalyzer::processMono (std::span<const float> samples,
                                    SpectrumFrameQueue& destination,
                                    RuntimeDiagnostics& diagnostics) noexcept
{
    for (const auto sample : samples)
        consumeSample (sample, destination, diagnostics);
}

void SpectrumAnalyzer::consumeSample (float sample,
                                      SpectrumFrameQueue& destination,
                                      RuntimeDiagnostics& diagnostics) noexcept
{
    inputRing[static_cast<std::size_t> (writePosition)] = sample;
    writePosition = (writePosition + 1) % configuredFftSize;

    if (filledSamples < configuredFftSize)
        ++filledSamples;
    else
        ++samplesSinceFrame;

    if (filledSamples == configuredFftSize
        && (! producedFirstFrame || samplesSinceFrame >= configuredHopSize))
    {
        produceFrame (destination, diagnostics);
        producedFirstFrame = true;
        samplesSinceFrame = 0;
    }
}

void SpectrumAnalyzer::produceFrame (SpectrumFrameQueue& destination,
                                     RuntimeDiagnostics& diagnostics) noexcept
{
    for (int sample = 0; sample < configuredFftSize; ++sample)
    {
        const auto ringIndex = (writePosition + sample) % configuredFftSize;
        fftData[static_cast<std::size_t> (sample)] =
            inputRing[static_cast<std::size_t> (ringIndex)] * window[static_cast<std::size_t> (sample)];
    }
    std::fill (fftData.begin() + configuredFftSize, fftData.end(), 0.0f);

    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    const auto commonScale = 1.0f / (static_cast<float> (configuredFftSize) * windowCoherentGain);
    for (int bin = 0; bin <= configuredFftSize / 2; ++bin)
    {
        const auto edgeScale = (bin == 0 || bin == configuredFftSize / 2) ? 1.0f : 2.0f;
        const auto magnitude = fftData[static_cast<std::size_t> (bin)] * commonScale * edgeScale;
        decibels[static_cast<std::size_t> (bin)] =
            juce::Decibels::gainToDecibels (magnitude, -100.0f);
    }

    const auto binWidth = sampleRate / configuredFftSize;
    SpectrumFrame frame;
    frame.bandCount = static_cast<std::uint16_t> (configuredBandCount);

    for (int band = 0; band < configuredBandCount; ++band)
    {
        const auto low = bandEdges[static_cast<std::size_t> (band)];
        const auto high = bandEdges[static_cast<std::size_t> (band + 1)];
        const auto firstBin = std::max (1, static_cast<int> (std::ceil (low / binWidth)));
        const auto lastBin = std::min (configuredFftSize / 2,
                                      static_cast<int> (std::floor (high / binWidth)));

        float bandDb = -100.0f;
        if (firstBin <= lastBin)
        {
            for (int bin = firstBin; bin <= lastBin; ++bin)
                bandDb = std::max (bandDb, decibels[static_cast<std::size_t> (bin)]);
        }
        else
        {
            bandDb = interpolateDecibels (std::sqrt (low * high));
        }

        bandDecibels[static_cast<std::size_t> (band)] = bandDb;
    }

    powerAverager.processDecibels (
        std::span<float> { bandDecibels.data(), bandDecibels.size() },
        static_cast<double> (configuredHopSize) / sampleRate);

    for (int band = 0; band < configuredBandCount; ++band)
    {
        const auto bandDb = bandDecibels[static_cast<std::size_t> (band)];
        frame.magnitudes[static_cast<std::size_t> (band)] =
            juce::jlimit (0.0f, 1.0f, (bandDb + 100.0f) / 100.0f);
    }

    diagnostics.fftFrames.fetch_add (1, std::memory_order_relaxed);
    if (destination.tryPush (frame))
        diagnostics.queuePushes.fetch_add (1, std::memory_order_relaxed);
    else
        diagnostics.queueDrops.fetch_add (1, std::memory_order_relaxed);
}

float SpectrumAnalyzer::interpolateDecibels (double frequency) const noexcept
{
    const auto fractionalBin = frequency * configuredFftSize / sampleRate;
    const auto lower = juce::jlimit (0, configuredFftSize / 2, static_cast<int> (std::floor (fractionalBin)));
    const auto upper = juce::jlimit (0, configuredFftSize / 2, lower + 1);
    const auto fraction = static_cast<float> (fractionalBin - lower);
    return juce::jmap (fraction,
                       decibels[static_cast<std::size_t> (lower)],
                       decibels[static_cast<std::size_t> (upper)]);
}
