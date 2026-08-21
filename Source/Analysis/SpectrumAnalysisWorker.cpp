#include "SpectrumAnalysisWorker.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

SpectrumAnalysisWorker::SpectrumAnalysisWorker (SpectrumFrameQueue& destination,
                                                RuntimeDiagnostics& runtimeDiagnostics)
    : juce::Thread ("Spectral Relief analysis"),
      frameQueue (destination),
      diagnostics (runtimeDiagnostics)
{
    normalAnalyzer.prepare (48000.0);
    highAnalyzer.prepare (48000.0);
    ultraAnalyzer.prepare (48000.0);
}

SpectrumAnalysisWorker::~SpectrumAnalysisWorker()
{
    visualizationClients.store (0, std::memory_order_release);
    stopIfNeeded();
}

void SpectrumAnalysisWorker::prepare (double sampleRate)
{
    const auto shouldRestart = isThreadRunning();
    if (shouldRestart)
        stopIfNeeded();

    sampleQueue.discard();
    frameQueue.requestDiscard();
    normalAnalyzer.prepare (sampleRate);
    highAnalyzer.prepare (sampleRate);
    ultraAnalyzer.prepare (sampleRate);
    const auto range = static_cast<FrequencyRange> (std::clamp (
        selectedRange.load (std::memory_order_acquire), 0, 3));
    const auto rangeProfile = getFrequencyRangeProfile (range);
    normalAnalyzer.setFrequencyRange (rangeProfile.minimumHz, rangeProfile.maximumHz);
    highAnalyzer.setFrequencyRange (rangeProfile.minimumHz, rangeProfile.maximumHz);
    ultraAnalyzer.setFrequencyRange (rangeProfile.minimumHz, rangeProfile.maximumHz);
    const auto averageMilliseconds = selectedAverageMilliseconds.load (std::memory_order_acquire);
    normalAnalyzer.setAverageMilliseconds (averageMilliseconds);
    highAnalyzer.setAverageMilliseconds (averageMilliseconds);
    ultraAnalyzer.setAverageMilliseconds (averageMilliseconds);
    diagnostics.analysisUpperHz.store (static_cast<float> (normalAnalyzer.getUpperFrequency()),
                                       std::memory_order_relaxed);

    if (shouldRestart && isVisualizationActive())
        startIfNeeded();
}

void SpectrumAnalysisWorker::capture (const juce::AudioBuffer<float>& audio) noexcept
{
    if (! isVisualizationActive())
        return;

    const auto channels = audio.getNumChannels();
    const auto samples = audio.getNumSamples();
    std::uint64_t accepted = 0;
    std::uint64_t dropped = 0;

    for (int sample = 0; sample < samples; ++sample)
    {
        float mono = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
            mono += audio.getReadPointer (channel)[sample];
        if (channels > 0)
            mono /= static_cast<float> (channels);

        if (sampleQueue.tryPush (mono))
            ++accepted;
        else
            ++dropped;
    }

    diagnostics.capturedSamples.fetch_add (accepted, std::memory_order_relaxed);
    diagnostics.sampleDrops.fetch_add (dropped, std::memory_order_relaxed);
}

void SpectrumAnalysisWorker::setVisualizationActive (bool shouldBeActive)
{
    if (shouldBeActive)
    {
        const auto previous = visualizationClients.fetch_add (1, std::memory_order_acq_rel);
        if (previous == 0)
            startIfNeeded();
        return;
    }

    auto current = visualizationClients.load (std::memory_order_acquire);
    while (current > 0
           && ! visualizationClients.compare_exchange_weak (current, current - 1,
                                                             std::memory_order_acq_rel))
    {
    }
    if (current == 1)
        stopIfNeeded();
}

void SpectrumAnalysisWorker::setAnalysisSelection (AnalysisQuality quality,
                                                   FrequencyRange range,
                                                   float averageMilliseconds) noexcept
{
    const auto qualityValue = static_cast<int> (quality);
    const auto rangeValue = static_cast<int> (range);
    const auto qualityChanged = selectedQuality.exchange (qualityValue, std::memory_order_acq_rel)
                             != qualityValue;
    const auto rangeChanged = selectedRange.exchange (rangeValue, std::memory_order_acq_rel)
                           != rangeValue;
    const auto safeAverage = std::isfinite (averageMilliseconds)
        ? std::clamp (averageMilliseconds, 0.0f, 1000.0f)
        : 0.0f;
    const auto previousAverage = selectedAverageMilliseconds.exchange (
        safeAverage, std::memory_order_acq_rel);
    if (previousAverage >= 0.5f && safeAverage < 0.5f)
        averagingResetPending.store (true, std::memory_order_release);
    diagnostics.setAverageSnapshot (
        safeAverage,
        static_cast<float> (PerceptualPowerAverager::attackSecondsForMilliseconds (safeAverage)),
        static_cast<float> (PerceptualPowerAverager::releaseSecondsForMilliseconds (safeAverage)));
    if (qualityChanged || rangeChanged)
        selectionRevision.fetch_add (1, std::memory_order_release);
}

void SpectrumAnalysisWorker::requestReset() noexcept
{
    analysisResetBoundary.store (sampleQueue.getWriteSequence(), std::memory_order_release);
    analysisResetPending.store (true, std::memory_order_release);
}

void SpectrumAnalysisWorker::run()
{
    std::array<float, 2048> batch {};
    std::array<std::uint64_t, 2048> batchSequences {};
    auto observedSelectionRevision = std::numeric_limits<std::uint32_t>::max();

    const auto resetAnalyzersAtBoundary = [this] (std::uint64_t boundary)
    {
        sampleQueue.discardBefore (boundary);
        frameQueue.requestDiscard();
        normalAnalyzer.reset();
        highAnalyzer.reset();
        ultraAnalyzer.reset();
    };

    while (! threadShouldExit())
    {
        if (analysisResetPending.exchange (false, std::memory_order_acq_rel))
            resetAnalyzersAtBoundary (
                analysisResetBoundary.load (std::memory_order_acquire));

        if (averagingResetPending.exchange (false, std::memory_order_acq_rel))
        {
            normalAnalyzer.resetAveraging();
            highAnalyzer.resetAveraging();
            ultraAnalyzer.resetAveraging();
        }

        const auto revision = selectionRevision.load (std::memory_order_acquire);
        if (revision != observedSelectionRevision)
        {
            observedSelectionRevision = revision;
            normalAnalyzer.reset();
            highAnalyzer.reset();
            ultraAnalyzer.reset();
            const auto range = static_cast<FrequencyRange> (std::clamp (
                selectedRange.load (std::memory_order_acquire), 0, 3));
            const auto rangeProfile = getFrequencyRangeProfile (range);
            normalAnalyzer.setFrequencyRange (rangeProfile.minimumHz, rangeProfile.maximumHz);
            highAnalyzer.setFrequencyRange (rangeProfile.minimumHz, rangeProfile.maximumHz);
            ultraAnalyzer.setFrequencyRange (rangeProfile.minimumHz, rangeProfile.maximumHz);
            diagnostics.analysisUpperHz.store (
                static_cast<float> (normalAnalyzer.getUpperFrequency()),
                std::memory_order_relaxed);
            frameQueue.requestDiscard();
        }

        std::size_t count = 0;
        while (count < batch.size()
               && sampleQueue.tryPop (batch[count], batchSequences[count]))
            ++count;

        if (count == 0)
        {
            juce::Thread::sleep (5);
            continue;
        }

        std::size_t processOffset = 0;
        for (;;)
        {
            if (analysisResetPending.exchange (false, std::memory_order_acq_rel))
            {
                const auto boundary = analysisResetBoundary.load (std::memory_order_acquire);
                resetAnalyzersAtBoundary (boundary);
                processOffset = static_cast<std::size_t> (std::distance (
                    batchSequences.begin(),
                    std::lower_bound (batchSequences.begin(),
                                      batchSequences.begin() + static_cast<std::ptrdiff_t> (count),
                                      boundary)));
            }

            if (processOffset < count)
            {
                auto* analyzer = &normalAnalyzer;
                switch (static_cast<AnalysisQuality> (std::clamp (
                    selectedQuality.load (std::memory_order_acquire), 0, 2)))
                {
                    case AnalysisQuality::ultra: analyzer = &ultraAnalyzer; break;
                    case AnalysisQuality::high:  analyzer = &highAnalyzer; break;
                    case AnalysisQuality::normal: break;
                }
                analyzer->setAverageMilliseconds (
                    selectedAverageMilliseconds.load (std::memory_order_acquire));
                analyzer->processMono (
                    std::span<const float> {
                        batch.data() + static_cast<std::ptrdiff_t> (processOffset),
                        count - processOffset },
                    frameQueue,
                    diagnostics);
            }

            if (! analysisResetPending.load (std::memory_order_acquire))
                break;
        }
    }
}

void SpectrumAnalysisWorker::startIfNeeded()
{
    if (! isThreadRunning())
        startThread (juce::Thread::Priority::low);
}

void SpectrumAnalysisWorker::stopIfNeeded()
{
    if (! isThreadRunning())
        return;

    signalThreadShouldExit();
    stopThread (1000);
}
