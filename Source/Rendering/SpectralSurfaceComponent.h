#pragma once

#include "DisplayRowProcessor.h"
#include "CameraProjection.h"
#include "FrequencyGuides.h"
#include "RenderRepaintDriver.h"
#include "SurfaceResources.h"
#include "SpectrumFrameAggregator.h"
#include "Source/Analysis/SpectrumFrameQueue.h"
#include "Source/Analysis/AnalysisQuality.h"
#include "Source/Diagnostics/RuntimeDiagnostics.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

class SpectralSurfaceComponent final : public juce::Component,
                                       private juce::OpenGLRenderer
{
public:
    static constexpr std::size_t historyTextureBytes = 1024 * 256 * sizeof (std::uint16_t);

    SpectralSurfaceComponent (SpectrumFrameQueue&,
                              RuntimeDiagnostics&,
                              juce::AudioProcessorValueTreeState&);
    ~SpectralSurfaceComponent() override;

    void paint (juce::Graphics&) override;
    void setHeld (bool shouldHold) noexcept { held.store (shouldHold); }
    void clearHistory() noexcept { clearRequested.store (true); }
    bool isRendererReady() const noexcept { return rendererReady.load(); }

private:
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    void uploadFrame (const SpectrumFrame&);
    void clearTexture();
    void createSurfaceResources (SurfaceResources&, int width, int depth);
    void deleteSurfaceResources (SurfaceResources&);
    void rebuildGuideResources();
    void deleteGuideResources();
    void resetDisplayState (int bandCount);
    void updateGuideLabels (const ProjectionParameters&, float aspect);
    void setStatus (juce::String);

    SpectrumFrameQueue& queue;
    RuntimeDiagnostics& diagnostics;
    std::atomic<float>* heightParameter = nullptr;
    std::atomic<float>* curveParameter = nullptr;
    std::atomic<float>* depthParameter = nullptr;
    std::atomic<float>* tiltParameter = nullptr;
    std::atomic<float>* orbitParameter = nullptr;
    std::atomic<float>* zoomParameter = nullptr;
    std::atomic<float>* contrastParameter = nullptr;
    std::atomic<float>* smoothingParameter = nullptr;
    std::atomic<float>* historyParameter = nullptr;
    std::atomic<float>* resolutionParameter = nullptr;
    std::atomic<float>* rangeParameter = nullptr;
    std::atomic<float>* view2dParameter = nullptr;

    juce::OpenGLContext openGLContext;
    RenderRepaintDriver repaintDriver;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram> guideShader;
    DisplayRowProcessor rowProcessor;
    SpectrumFrameAggregator frameAggregator;
    HistoryCadence cadence;

    SurfaceResources surfaceResources;
    GLuint historyTexture = 0;
    int meshDataAttribute = -1;
    int textureUniform = -1;
    int writeOffsetUniform = -1;
    int heightScaleUniform = -1;
    int cameraMatrixUniform = -1;
    int lensAmountUniform = -1;
    int depthAmountUniform = -1;
    int viewportAspectUniform = -1;
    int zoomAmountUniform = -1;
    int contrastAmountUniform = -1;
    int view2dAmountUniform = -1;
    int texelSizeUniform = -1;
    int frequencyScaleUniform = -1;
    GLuint guideVertexArray = 0;
    GLuint guideVertexBuffer = 0;
    int guideVertexCount = 0;
    int guideDataAttribute = -1;
    int guideCameraMatrixUniform = -1;
    int guideLensAmountUniform = -1;
    int guideDepthAmountUniform = -1;
    int guideViewportAspectUniform = -1;
    int guideZoomAmountUniform = -1;
    int guideView2dAmountUniform = -1;
    int writeRow = 0;
    double lastRenderMilliseconds = 0.0;
    double lastSmoothedFrameMilliseconds = 0.0;
    juce::SmoothedValue<float> smoothedHeight;
    juce::SmoothedValue<float> smoothedLens;
    juce::SmoothedValue<float> smoothedDepth;
    juce::SmoothedValue<float> smoothedTilt;
    juce::SmoothedValue<float> smoothedOrbit;
    juce::SmoothedValue<float> smoothedZoom;
    juce::SmoothedValue<float> smoothedContrast;
    std::array<float, maximumSpectrumBandCount * 256> clearPixels {};
    AnalysisQuality activeQuality = AnalysisQuality::normal;
    FrequencyRange activeRange = FrequencyRange::full;
    FrequencyGuideSet activeGuides;
    std::array<std::atomic<float>, 10> guideLabelX {};
    std::array<std::atomic<float>, 10> guideLabelY {};
    std::array<std::atomic<int>, 10> guideLabelFrequency {};
    std::atomic<int> guideLabelCount { 0 };

    std::atomic<bool> held { false };
    std::atomic<bool> clearRequested { false };
    std::atomic<bool> rendererReady { false };
    juce::CriticalSection statusLock;
    juce::String statusText;
};
