#pragma once

#include "Source/Rendering/CameraProjection.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <cstdint>

struct RuntimeDiagnostics
{
    std::atomic<std::uint64_t> processBlocks { 0 };
    std::atomic<std::uint64_t> processedSamples { 0 };
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<std::uint64_t> capturedSamples { 0 };
    std::atomic<std::uint64_t> sampleDrops { 0 };
    std::atomic<std::uint64_t> fftFrames { 0 };
    std::atomic<std::uint64_t> queuePushes { 0 };
    std::atomic<std::uint64_t> queueDrops { 0 };
    std::atomic<std::uint64_t> renderCalls { 0 };
    std::atomic<std::uint64_t> queuePops { 0 };
    std::atomic<std::uint64_t> rowsUploaded { 0 };
    std::atomic<float> uploadedPeak { 0.0f };
    std::atomic<unsigned int> lastGlError { 0 };
    std::atomic<bool> shaderReady { false };
    std::atomic<int> meshAttribute { -1 };
    std::atomic<int> textureUniform { -1 };
    std::atomic<float> projectionHeight { 1.4f };
    std::atomic<float> projectionLens { 0.35f };
    std::atomic<float> projectionDepth { 1.0f };
    std::atomic<float> projectionTilt { 55.0f };
    std::atomic<float> projectionOrbit { -18.0f };
    std::atomic<float> projectionZoom { 1.0f };
    std::atomic<float> projectionContrast { 1.0f };
    std::atomic<bool> displayView2d { false };
    std::atomic<int> displayResolution { 0 };
    std::atomic<int> displayRange { 0 };
    std::atomic<int> displayBands { 256 };
    std::atomic<int> displayGuides { 10 };
    std::atomic<float> displaySmoothTau { 0.0f };
    std::atomic<float> analysisUpperHz { 20000.0f };
    std::atomic<float> analysisAverageMilliseconds { 80.0f };
    std::atomic<float> analysisAverageAttackSeconds { 0.020f };
    std::atomic<float> analysisAverageReleaseSeconds { 0.080f };

    void recordInputPeak (float peak) noexcept { recordMaximum (inputPeak, peak); }
    void recordUploadedPeak (float peak) noexcept { recordMaximum (uploadedPeak, peak); }

    void setProjectionSnapshot (const ProjectionParameters& values) noexcept
    {
        projectionHeight.store (values.height, std::memory_order_relaxed);
        projectionLens.store (values.lens, std::memory_order_relaxed);
        projectionDepth.store (values.depth, std::memory_order_relaxed);
        projectionTilt.store (values.tiltDegrees, std::memory_order_relaxed);
        projectionOrbit.store (values.orbitDegrees, std::memory_order_relaxed);
        projectionZoom.store (values.zoom, std::memory_order_relaxed);
        projectionContrast.store (values.contrast, std::memory_order_relaxed);
    }

    void setDisplaySnapshot (bool view2d,
                             int resolution,
                             int range,
                             int bands,
                             int guides,
                             float smoothTau) noexcept
    {
        displayView2d.store (view2d, std::memory_order_relaxed);
        displayResolution.store (resolution, std::memory_order_relaxed);
        displayRange.store (range, std::memory_order_relaxed);
        displayBands.store (bands, std::memory_order_relaxed);
        displayGuides.store (guides, std::memory_order_relaxed);
        displaySmoothTau.store (smoothTau, std::memory_order_relaxed);
    }

    void setAverageSnapshot (float milliseconds,
                             float attackSeconds,
                             float releaseSeconds) noexcept
    {
        analysisAverageMilliseconds.store (milliseconds, std::memory_order_relaxed);
        analysisAverageAttackSeconds.store (attackSeconds, std::memory_order_relaxed);
        analysisAverageReleaseSeconds.store (releaseSeconds, std::memory_order_relaxed);
    }

    void setOpenGLInfo (juce::String newInfo)
    {
        const juce::ScopedLock lock (textLock);
        openGLInfo = std::move (newInfo);
    }

    void setShaderStatus (juce::String newStatus)
    {
        const juce::ScopedLock lock (textLock);
        shaderStatus = std::move (newStatus);
    }

    juce::String snapshotLine (bool resetPeaks = false)
    {
        const auto input = resetPeaks ? inputPeak.exchange (0.0f) : inputPeak.load();
        const auto uploaded = resetPeaks ? uploadedPeak.exchange (0.0f) : uploadedPeak.load();
        juce::String glInfo;
        juce::String shaderInfo;
        {
            const juce::ScopedLock lock (textLock);
            glInfo = openGLInfo;
            shaderInfo = shaderStatus;
        }

        return "blocks=" + juce::String (processBlocks.load())
             + " samples=" + juce::String (processedSamples.load())
             + " inputPeak=" + juce::String (input, 3)
             + " capture=" + juce::String (capturedSamples.load())
             + " sampleDrop=" + juce::String (sampleDrops.load())
             + " fft=" + juce::String (fftFrames.load())
             + " push=" + juce::String (queuePushes.load())
             + " drop=" + juce::String (queueDrops.load())
             + " render=" + juce::String (renderCalls.load())
             + " pop=" + juce::String (queuePops.load())
             + " upload=" + juce::String (rowsUploaded.load())
             + " uploadPeak=" + juce::String (uploaded, 3)
             + " shader=" + juce::String (shaderReady.load() ? 1 : 0)
             + " attr=" + juce::String (meshAttribute.load())
             + " texUniform=" + juce::String (textureUniform.load())
             + " glError=" + juce::String (lastGlError.load())
             + " height=" + juce::String (projectionHeight.load(), 3)
             + " lens=" + juce::String (projectionLens.load(), 3)
             + " depth=" + juce::String (projectionDepth.load(), 3)
             + " tilt=" + juce::String (projectionTilt.load(), 3)
             + " orbit=" + juce::String (projectionOrbit.load(), 3)
             + " zoom=" + juce::String (projectionZoom.load(), 3)
             + " contrast=" + juce::String (projectionContrast.load(), 3)
             + " view=" + juce::String (displayView2d.load() ? "2D" : "3D")
             + " resolution=" + juce::String (displayResolution.load())
             + " range=" + juce::String (displayRange.load())
             + " bands=" + juce::String (displayBands.load())
             + " guides=" + juce::String (displayGuides.load())
             + " smoothTau=" + juce::String (displaySmoothTau.load(), 3)
             + " averageMs=" + juce::String (analysisAverageMilliseconds.load(), 1)
             + " averageAttack=" + juce::String (analysisAverageAttackSeconds.load(), 3)
             + " averageRelease=" + juce::String (analysisAverageReleaseSeconds.load(), 3)
             + " upperHz=" + juce::String (analysisUpperHz.load(), 1)
             + " gl={" + glInfo + "} shaderStatus={" + shaderInfo + "}";
    }

private:
    static void recordMaximum (std::atomic<float>& destination, float candidate) noexcept
    {
        auto current = destination.load (std::memory_order_relaxed);
        while (candidate > current
               && ! destination.compare_exchange_weak (current, candidate,
                                                       std::memory_order_relaxed))
        {
        }
    }

    juce::CriticalSection textLock;
    juce::String openGLInfo;
    juce::String shaderStatus;
};

class RuntimeDiagnosticsLogger final : private juce::Timer
{
public:
    explicit RuntimeDiagnosticsLogger (RuntimeDiagnostics& source)
#if JUCE_DEBUG
        : diagnostics (source),
#else
        :
#endif
          logFile (juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                       .getChildFile ("Library")
                       .getChildFile ("Logs")
                       .getChildFile ("SpectralRelief.log"))
    {
#if JUCE_DEBUG
        static_cast<void> (logFile.getParentDirectory().createDirectory());
        static_cast<void> (logFile.appendText ("\n--- Spectral Relief editor opened "
                                                + juce::Time::getCurrentTime().toISO8601 (true)
                                                + " ---\n"));
        startTimerHz (1);
#else
        juce::ignoreUnused (source);
#endif
    }

    ~RuntimeDiagnosticsLogger() override { stopTimer(); }

    const juce::File& getLogFile() const noexcept { return logFile; }

private:
    void timerCallback() override
    {
#if JUCE_DEBUG
        const auto line = juce::Time::getCurrentTime().formatted ("%H:%M:%S")
                        + " " + diagnostics.snapshotLine (true);
        static_cast<void> (logFile.appendText (line + "\n"));
        juce::Logger::writeToLog ("SpectralRelief " + line);
#endif
    }

#if JUCE_DEBUG
    RuntimeDiagnostics& diagnostics;
#endif
    juce::File logFile;
};
