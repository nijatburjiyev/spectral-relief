#pragma once

#include "AnalysisQuality.h"
#include "AudioSampleQueue.h"
#include "FrequencyRange.h"
#include "SpectrumAnalyzer.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <array>
#include <atomic>
#include <cstdint>

class SpectrumAnalysisWorker final : private juce::Thread
{
public:
    SpectrumAnalysisWorker (SpectrumFrameQueue&, RuntimeDiagnostics&);
    ~SpectrumAnalysisWorker() override;

    void prepare (double sampleRate);
    void capture (const juce::AudioBuffer<float>&) noexcept;
    void setVisualizationActive (bool);
    void setAnalysisSelection (AnalysisQuality, FrequencyRange, float averageMilliseconds) noexcept;
    void requestReset() noexcept;

    [[nodiscard]] bool isVisualizationActive() const noexcept
    {
        return visualizationClients.load (std::memory_order_acquire) > 0;
    }

private:
    void run() override;
    void startIfNeeded();
    void stopIfNeeded();

    SpectrumFrameQueue& frameQueue;
    RuntimeDiagnostics& diagnostics;
    AudioSampleQueue sampleQueue;
    SpectrumAnalyzer normalAnalyzer { 11, 256, 256 };
    SpectrumAnalyzer highAnalyzer { 12, 512, 256 };
    SpectrumAnalyzer ultraAnalyzer { 13, 1024, 256 };
    std::atomic<int> visualizationClients { 0 };
    std::atomic<int> selectedQuality { static_cast<int> (AnalysisQuality::normal) };
    std::atomic<int> selectedRange { static_cast<int> (FrequencyRange::full) };
    std::atomic<float> selectedAverageMilliseconds { 80.0f };
    std::atomic<std::uint64_t> analysisResetBoundary { 0 };
    std::atomic<bool> analysisResetPending { false };
    std::atomic<bool> averagingResetPending { false };
    std::atomic<std::uint32_t> selectionRevision { 0 };
};
