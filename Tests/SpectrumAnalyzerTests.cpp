#include "Source/Analysis/SpectrumAnalyzer.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
constexpr double sampleRate = 48000.0;

SpectrumRow analyze (const std::vector<std::vector<float>>& channels)
{
    SpectrumAnalyzer analyzer;
    SpectrumFrameQueue queue;
    RuntimeDiagnostics diagnostics;
    analyzer.prepare (sampleRate);

    std::array<const float*, 2> pointers {};
    for (std::size_t channel = 0; channel < channels.size(); ++channel)
        pointers[channel] = channels[channel].data();

    analyzer.process (pointers.data(),
                      static_cast<int> (channels.size()),
                      static_cast<int> (channels.front().size()),
                      queue,
                      diagnostics);

    SpectrumRow current {};
    SpectrumRow latest {};
    while (queue.tryPop (current))
        latest = current;
    return latest;
}

std::vector<float> sine (double frequency, float amplitude, int samples)
{
    std::vector<float> result (static_cast<std::size_t> (samples));
    for (int sample = 0; sample < samples; ++sample)
        result[static_cast<std::size_t> (sample)] =
            amplitude * static_cast<float> (
                            std::sin (juce::MathConstants<double>::twoPi
                                      * frequency * static_cast<double> (sample) / sampleRate));
    return result;
}

std::size_t expectedBand (double frequency)
{
    const auto proportion = std::log (frequency / 20.0) / std::log (20000.0 / 20.0);
    return static_cast<std::size_t> (juce::jlimit (0, 255, static_cast<int> (proportion * 256.0)));
}

SpectrumFrame takeLatest (SpectrumFrameQueue& queue)
{
    SpectrumFrame frame;
    SpectrumFrame latest;
    while (queue.tryPop (frame))
        latest = frame;
    return latest;
}

class SpectrumAnalyzerTests final : public juce::UnitTest
{
public:
    SpectrumAnalyzerTests() : juce::UnitTest ("Spectrum analyzer", "analysis") {}

    void runTest() override
    {
        constexpr int samples = SpectrumAnalyzer::fftSize + 2 * SpectrumAnalyzer::hopSize;

        beginTest ("silence stays at the display floor");
        const auto silent = analyze ({ std::vector<float> (samples, 0.0f) });
        for (const auto value : silent)
            expectWithinAbsoluteError (value, 0.0f, 1.0e-7f);

        beginTest ("Hann gain is calibrated for a bin-centred sine");
        constexpr double binCentredFrequency = sampleRate / SpectrumAnalyzer::fftSize * 20.0;
        const auto calibrated = analyze ({ sine (binCentredFrequency, 0.5f, samples) });
        const auto maximum = *std::max_element (calibrated.begin(), calibrated.end());
        const auto expectedNormalizedDb = static_cast<float> ((20.0 * std::log10 (0.5) + 100.0) / 100.0);
        expectWithinAbsoluteError (maximum, expectedNormalizedDb, 0.005f);

        beginTest ("power averaging slows decay without moving a harmonic");
        SpectrumAnalyzer rawAnalyzer;
        SpectrumAnalyzer averagedAnalyzer;
        rawAnalyzer.prepare (sampleRate);
        averagedAnalyzer.prepare (sampleRate);
        rawAnalyzer.setAverageMilliseconds (0.0f);
        averagedAnalyzer.setAverageMilliseconds (500.0f);
        SpectrumFrameQueue rawQueue;
        SpectrumFrameQueue averagedQueue;
        RuntimeDiagnostics rawDiagnostics;
        RuntimeDiagnostics averagedDiagnostics;
        const auto loudTone = sine (binCentredFrequency, 1.0f,
                                    SpectrumAnalyzer::fftSize + 4 * SpectrumAnalyzer::hopSize);
        rawAnalyzer.processMono (loudTone, rawQueue, rawDiagnostics);
        averagedAnalyzer.processMono (loudTone, averagedQueue, averagedDiagnostics);
        takeLatest (rawQueue);
        takeLatest (averagedQueue);
        const auto quietTone = sine (binCentredFrequency, 0.1f,
                                     SpectrumAnalyzer::fftSize + SpectrumAnalyzer::hopSize);
        rawAnalyzer.processMono (quietTone, rawQueue, rawDiagnostics);
        averagedAnalyzer.processMono (quietTone, averagedQueue, averagedDiagnostics);
        const auto rawDrop = takeLatest (rawQueue);
        const auto averagedDrop = takeLatest (averagedQueue);
        const auto rawPeak = static_cast<std::size_t> (std::distance (
            rawDrop.magnitudes.begin(),
            std::max_element (rawDrop.magnitudes.begin(),
                              rawDrop.magnitudes.begin() + rawDrop.bandCount)));
        const auto averagedPeak = static_cast<std::size_t> (std::distance (
            averagedDrop.magnitudes.begin(),
            std::max_element (averagedDrop.magnitudes.begin(),
                              averagedDrop.magnitudes.begin() + averagedDrop.bandCount)));
        expectEquals (averagedPeak, rawPeak);
        expect (averagedDrop.magnitudes[averagedPeak] > rawDrop.magnitudes[rawPeak] + 0.05f);

        beginTest ("analyzer reset removes averaging history");
        averagedAnalyzer.reset();
        SpectrumFrameQueue resetQueue;
        averagedAnalyzer.processMono (quietTone, resetQueue, averagedDiagnostics);
        const auto afterReset = takeLatest (resetQueue);
        const auto resetMaximum = *std::max_element (
            afterReset.magnitudes.begin(), afterReset.magnitudes.begin() + afterReset.bandCount);
        expectWithinAbsoluteError (resetMaximum, 0.8f, 0.01f);

        beginTest ("an off-bin 440 Hz sine is localized");
        const auto tone440 = analyze ({ sine (440.0, 0.8f, samples) });
        const auto peak = static_cast<std::size_t> (
            std::distance (tone440.begin(), std::max_element (tone440.begin(), tone440.end())));
        const auto target = expectedBand (440.0);
        expect (peak >= target - 1 && peak <= target + 1);

        beginTest ("separated narrow tones remain distinct");
        auto low = sine (440.0, 0.4f, samples);
        const auto high = sine (1760.0, 0.4f, samples);
        for (std::size_t i = 0; i < low.size(); ++i)
            low[i] += high[i];
        const auto twoTones = analyze ({ low });
        const auto lowBand = expectedBand (440.0);
        const auto highBand = expectedBand (1760.0);
        const auto localMaximum = [&twoTones] (std::size_t centre)
        {
            return *std::max_element (twoTones.begin() + static_cast<std::ptrdiff_t> (centre - 2),
                                      twoTones.begin() + static_cast<std::ptrdiff_t> (centre + 3));
        };
        expect (localMaximum (lowBand) > 0.88f);
        expect (localMaximum (highBand) > 0.88f);

        beginTest ("stereo analysis uses an arithmetic downmix");
        auto left = sine (440.0, 0.8f, samples);
        auto right = left;
        for (auto& value : right)
            value = -value;
        const auto cancelled = analyze ({ left, right });
        expect (*std::max_element (cancelled.begin(), cancelled.end()) < 0.001f);

        beginTest ("supported sample rates produce finite rows");
        for (const auto rate : { 32000.0, 44100.0, 48000.0, 96000.0 })
        {
            SpectrumAnalyzer analyzer;
            analyzer.prepare (rate);
            expect (analyzer.getUpperFrequency() <= std::min (20000.0, rate * 0.5));
            expect (analyzer.getUpperFrequency() >= 16000.0);
        }

        beginTest ("Ultra focused Low range uses all 1024 calibrated bands");
        SpectrumAnalyzer ultra (13, 1024, 256);
        ultra.prepare (sampleRate);
        ultra.setFrequencyRange (20.0, 500.0);
        const auto focusedTone = sine (100.0, 0.5f, 8192 + 512);
        SpectrumFrameQueue focusedQueue;
        RuntimeDiagnostics focusedDiagnostics;
        ultra.processMono (focusedTone, focusedQueue, focusedDiagnostics);
        SpectrumFrame focusedFrame;
        SpectrumFrame latestFocused;
        while (focusedQueue.tryPop (focusedFrame))
            latestFocused = focusedFrame;
        expectEquals (latestFocused.bandCount, static_cast<std::uint16_t> (1024));
        const auto focusedPeak = static_cast<int> (std::distance (
            latestFocused.magnitudes.begin(),
            std::max_element (latestFocused.magnitudes.begin(),
                              latestFocused.magnitudes.begin() + latestFocused.bandCount)));
        const auto expectedFocusedBand = static_cast<int> (
            std::log (100.0 / 20.0) / std::log (500.0 / 20.0) * 1024.0);
        expect (std::abs (focusedPeak - expectedFocusedBand) <= 2);
        expectWithinAbsoluteError (
            *std::max_element (latestFocused.magnitudes.begin(),
                               latestFocused.magnitudes.begin() + latestFocused.bandCount),
            static_cast<float> ((20.0 * std::log10 (0.5) + 100.0) / 100.0),
            0.01f);
    }
};

SpectrumAnalyzerTests spectrumAnalyzerTests;
} // namespace
