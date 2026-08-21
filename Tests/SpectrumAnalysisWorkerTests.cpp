#include "Source/Analysis/SpectrumAnalysisWorker.h"

#include <juce_core/juce_core.h>

#include <cmath>
#include <algorithm>

namespace
{
juce::AudioBuffer<float> makeHarmonicSignal (int samples)
{
    juce::AudioBuffer<float> audio (2, samples);
    for (int sample = 0; sample < samples; ++sample)
    {
        const auto time = static_cast<float> (sample) / 48000.0f;
        const auto value = 0.25f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * time)
                         + 0.20f * std::sin (juce::MathConstants<float>::twoPi * 880.0f * time)
                         + 0.15f * std::sin (juce::MathConstants<float>::twoPi * 1760.0f * time);
        audio.setSample (0, sample, value);
        audio.setSample (1, sample, value);
    }
    return audio;
}

bool waitForFrame (SpectrumFrameQueue& queue, SpectrumFrame& frame, int expectedBands)
{
    for (int attempt = 0; attempt < 250; ++attempt)
    {
        while (queue.tryPop (frame))
            if (frame.bandCount == expectedBands)
                return true;
        juce::Thread::sleep (2);
    }
    return false;
}

SpectrumFrame captureLatest (SpectrumAnalysisWorker& worker,
                             SpectrumFrameQueue& queue,
                             const juce::AudioBuffer<float>& audio,
                             int expectedBands)
{
    worker.capture (audio);
    SpectrumFrame frame;
    SpectrumFrame latest;
    bool received = false;
    int idlePasses = 0;
    for (int attempt = 0; attempt < 250 && idlePasses < 20; ++attempt)
    {
        bool receivedThisPass = false;
        while (queue.tryPop (frame))
        {
            if (frame.bandCount == expectedBands)
            {
                latest = frame;
                received = true;
                receivedThisPass = true;
            }
        }
        idlePasses = received && ! receivedThisPass ? idlePasses + 1 : 0;
        juce::Thread::sleep (2);
    }
    return latest;
}

juce::AudioBuffer<float> makeTone (float amplitude,
                                   float frequency = 468.75f,
                                   int samples = 4096)
{
    juce::AudioBuffer<float> audio (2, samples);
    for (int sample = 0; sample < audio.getNumSamples(); ++sample)
    {
        const auto value = amplitude * std::sin (juce::MathConstants<float>::twoPi
                                                  * frequency * static_cast<float> (sample)
                                                  / 48000.0f);
        audio.setSample (0, sample, value);
        audio.setSample (1, sample, value);
    }
    return audio;
}

struct WorkerDrop
{
    float magnitude = 0.0f;
    std::size_t peakBand = 0;
};

WorkerDrop measureWorkerDrop (float averageMilliseconds)
{
    SpectrumFrameQueue frames;
    RuntimeDiagnostics diagnostics;
    SpectrumAnalysisWorker worker (frames, diagnostics);
    worker.prepare (48000.0);
    worker.setAnalysisSelection (AnalysisQuality::normal,
                                 FrequencyRange::full,
                                 averageMilliseconds);
    worker.setVisualizationActive (true);
    captureLatest (worker, frames, makeTone (1.0f), 256);
    const auto quiet = captureLatest (worker, frames, makeTone (0.1f), 256);
    worker.setVisualizationActive (false);
    const auto peak = std::max_element (quiet.magnitudes.begin(),
                                        quiet.magnitudes.begin() + quiet.bandCount);
    return { *peak, static_cast<std::size_t> (std::distance (quiet.magnitudes.begin(), peak)) };
}

std::size_t expectedBand (double frequency, int bandCount)
{
    const auto proportion = std::log (frequency / 20.0) / std::log (20000.0 / 20.0);
    return static_cast<std::size_t> (juce::jlimit (0, bandCount - 1,
                                                   static_cast<int> (proportion * bandCount)));
}

class SpectrumAnalysisWorkerTests final : public juce::UnitTest
{
public:
    SpectrumAnalysisWorkerTests() : juce::UnitTest ("Spectrum analysis worker", "analysis") {}

    void runTest() override
    {
        beginTest ("selection made while stopped is applied on first worker iteration");
        SpectrumFrameQueue stoppedFrames;
        RuntimeDiagnostics stoppedDiagnostics;
        SpectrumAnalysisWorker stoppedWorker (stoppedFrames, stoppedDiagnostics);
        stoppedWorker.prepare (48000.0);
        stoppedWorker.setAnalysisSelection (AnalysisQuality::ultra,
                                            FrequencyRange::low, 500.0f);
        stoppedWorker.setVisualizationActive (true);
        const auto stoppedLow = captureLatest (
            stoppedWorker, stoppedFrames, makeTone (0.5f, 100.0f, 8704), 1024);
        const auto stoppedLowPeak = static_cast<int> (std::distance (
            stoppedLow.magnitudes.begin(),
            std::max_element (stoppedLow.magnitudes.begin(),
                              stoppedLow.magnitudes.begin() + stoppedLow.bandCount)));
        const auto expectedStoppedLow = static_cast<int> (
            std::log (100.0 / 20.0) / std::log (500.0 / 20.0) * 1024.0);
        expect (std::abs (stoppedLowPeak - expectedStoppedLow) <= 2);
        stoppedWorker.setVisualizationActive (false);

        stoppedWorker.setAnalysisSelection (AnalysisQuality::normal,
                                            FrequencyRange::high, 500.0f);
        stoppedWorker.setVisualizationActive (true);
        const auto stoppedHigh = captureLatest (
            stoppedWorker, stoppedFrames, makeTone (0.5f, 5000.0f), 256);
        const auto stoppedHighPeak = static_cast<int> (std::distance (
            stoppedHigh.magnitudes.begin(),
            std::max_element (stoppedHigh.magnitudes.begin(),
                              stoppedHigh.magnitudes.begin() + stoppedHigh.bandCount)));
        const auto expectedStoppedHigh = static_cast<int> (
            std::log (5000.0 / 2000.0) / std::log (20000.0 / 2000.0) * 256.0);
        expect (std::abs (stoppedHighPeak - expectedStoppedHigh) <= 2);
        stoppedWorker.setVisualizationActive (false);

        SpectrumFrameQueue frames;
        RuntimeDiagnostics diagnostics;
        SpectrumAnalysisWorker worker (frames, diagnostics);
        worker.prepare (48000.0);
        worker.setVisualizationActive (true);

        beginTest ("Normal worker publishes 256-band frames");
        auto signal = makeHarmonicSignal (8192);
        worker.setAnalysisSelection (AnalysisQuality::normal, FrequencyRange::full, 0.0f);
        expectWithinAbsoluteError (diagnostics.analysisAverageMilliseconds.load(), 0.0f, 1.0e-7f);
        expectWithinAbsoluteError (diagnostics.analysisAverageAttackSeconds.load(), 0.0f, 1.0e-7f);
        expectWithinAbsoluteError (diagnostics.analysisAverageReleaseSeconds.load(), 0.0f, 1.0e-7f);
        worker.capture (signal);
        SpectrumFrame normalFrame;
        expect (waitForFrame (frames, normalFrame, 256));
        expectEquals (normalFrame.bandCount, static_cast<std::uint16_t> (256));

        beginTest ("High worker publishes detailed 512-band frames");
        frames.requestDiscard();
        frames.serviceDiscardRequest();
        worker.setAnalysisSelection (AnalysisQuality::high, FrequencyRange::full, 0.0f);
        worker.capture (signal);
        SpectrumFrame highFrame;
        expect (waitForFrame (frames, highFrame, 512));
        expectEquals (highFrame.bandCount, static_cast<std::uint16_t> (512));
        for (const auto frequency : { 440.0, 880.0, 1760.0 })
        {
            const auto centre = expectedBand (frequency, 512);
            const auto peak = *std::max_element (
                highFrame.magnitudes.begin() + static_cast<std::ptrdiff_t> (centre - 2),
                highFrame.magnitudes.begin() + static_cast<std::ptrdiff_t> (centre + 3));
            expect (peak > 0.75f, "missing separated harmonic near " + juce::String (frequency));
        }

        beginTest ("Ultra worker publishes 1024 focused-range bands");
        frames.requestDiscard();
        frames.serviceDiscardRequest();
        worker.setAnalysisSelection (AnalysisQuality::ultra, FrequencyRange::low, 0.0f);
        juce::AudioBuffer<float> lowTone (2, 8704);
        for (int sample = 0; sample < lowTone.getNumSamples(); ++sample)
        {
            const auto value = 0.5f * std::sin (juce::MathConstants<float>::twoPi
                                                * 100.0f * static_cast<float> (sample) / 48000.0f);
            lowTone.setSample (0, sample, value);
            lowTone.setSample (1, sample, value);
        }
        worker.capture (lowTone);
        SpectrumFrame ultraFrame;
        expect (waitForFrame (frames, ultraFrame, 1024));
        expectEquals (ultraFrame.bandCount, static_cast<std::uint16_t> (1024));
        const auto expected100Hz = static_cast<int> (
            std::log (100.0 / 20.0) / std::log (500.0 / 20.0) * 1024.0);
        const auto peak100 = static_cast<int> (std::distance (
            ultraFrame.magnitudes.begin(),
            std::max_element (ultraFrame.magnitudes.begin(),
                              ultraFrame.magnitudes.begin() + ultraFrame.bandCount)));
        expect (std::abs (peak100 - expected100Hz) <= 2);

        worker.setVisualizationActive (false);

        beginTest ("worker transports averaging without changing peak frequency data");
        const auto rawDrop = measureWorkerDrop (0.0f);
        const auto averagedDrop = measureWorkerDrop (500.0f);
        expectEquals (averagedDrop.peakBand, rawDrop.peakBand);
        expect (averagedDrop.magnitude > rawDrop.magnitude + 0.05f);

        beginTest ("an OFF pulse durably clears averaging before the next batch");
        SpectrumFrameQueue pulseFrames;
        RuntimeDiagnostics pulseDiagnostics;
        SpectrumAnalysisWorker pulseWorker (pulseFrames, pulseDiagnostics);
        pulseWorker.prepare (48000.0);
        pulseWorker.setAnalysisSelection (AnalysisQuality::normal,
                                          FrequencyRange::full, 500.0f);
        pulseWorker.setVisualizationActive (true);
        captureLatest (pulseWorker, pulseFrames, makeTone (1.0f), 256);
        const auto retained = captureLatest (pulseWorker, pulseFrames, makeTone (0.1f), 256);
        const auto retainedPeak = *std::max_element (
            retained.magnitudes.begin(), retained.magnitudes.begin() + retained.bandCount);
        expect (retainedPeak > 0.9f);
        pulseWorker.setAnalysisSelection (AnalysisQuality::normal,
                                          FrequencyRange::full, 0.0f);
        pulseWorker.setAnalysisSelection (AnalysisQuality::normal,
                                          FrequencyRange::full, 500.0f);
        const auto afterPulse = captureLatest (
            pulseWorker, pulseFrames, makeTone (0.1f), 256);
        const auto afterPulsePeak = *std::max_element (
            afterPulse.magnitudes.begin(), afterPulse.magnitudes.begin() + afterPulse.bandCount);
        expectWithinAbsoluteError (afterPulsePeak, 0.8f, 0.02f);

        beginTest ("analysis Reset removes retained averaging power");
        captureLatest (pulseWorker, pulseFrames, makeTone (1.0f), 256);
        const auto beforeReset = captureLatest (
            pulseWorker, pulseFrames, makeTone (0.1f), 256);
        const auto beforeResetPeak = *std::max_element (
            beforeReset.magnitudes.begin(), beforeReset.magnitudes.begin() + beforeReset.bandCount);
        expect (beforeResetPeak > 0.9f);
        pulseWorker.requestReset();
        const auto afterReset = captureLatest (
            pulseWorker, pulseFrames, makeTone (0.1f), 256);
        const auto afterResetPeak = *std::max_element (
            afterReset.magnitudes.begin(), afterReset.magnitudes.begin() + afterReset.bandCount);
        expectWithinAbsoluteError (afterResetPeak, 0.8f, 0.02f);

        beginTest ("Resolution and Range changes clear averaging history");
        captureLatest (pulseWorker, pulseFrames, makeTone (1.0f), 256);
        captureLatest (pulseWorker, pulseFrames, makeTone (0.1f), 256);
        pulseWorker.setAnalysisSelection (AnalysisQuality::high,
                                          FrequencyRange::full, 500.0f);
        const auto afterResolution = captureLatest (
            pulseWorker, pulseFrames, makeTone (0.1f), 512);
        const auto afterResolutionPeak = *std::max_element (
            afterResolution.magnitudes.begin(),
            afterResolution.magnitudes.begin() + afterResolution.bandCount);
        expectWithinAbsoluteError (afterResolutionPeak, 0.8f, 0.02f);

        captureLatest (pulseWorker, pulseFrames, makeTone (1.0f), 512);
        captureLatest (pulseWorker, pulseFrames, makeTone (0.1f), 512);
        pulseWorker.setAnalysisSelection (AnalysisQuality::high,
                                          FrequencyRange::low, 500.0f);
        const auto afterRange = captureLatest (
            pulseWorker, pulseFrames, makeTone (0.1f), 512);
        const auto afterRangePeak = *std::max_element (
            afterRange.magnitudes.begin(), afterRange.magnitudes.begin() + afterRange.bandCount);
        expectWithinAbsoluteError (afterRangePeak, 0.8f, 0.02f);
        pulseWorker.setVisualizationActive (false);
    }
};

SpectrumAnalysisWorkerTests spectrumAnalysisWorkerTests;
} // namespace
