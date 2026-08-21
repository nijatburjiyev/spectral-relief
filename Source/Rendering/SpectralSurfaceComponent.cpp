#include "SpectralSurfaceComponent.h"

#include "Source/Parameters.h"
#include "SpectralShaders.h"
#include "SurfaceMesh.h"

#include <algorithm>
#include <array>
#include <vector>

namespace
{
juce::String formatFrequency (int frequency)
{
    if (frequency >= 1000)
        return frequency % 1000 == 0
            ? juce::String (frequency / 1000) + " kHz"
            : juce::String (static_cast<double> (frequency) / 1000.0, 1) + " kHz";
    return juce::String (frequency) + " Hz";
}
}

SpectralSurfaceComponent::SpectralSurfaceComponent (
    SpectrumFrameQueue& sourceQueue,
    RuntimeDiagnostics& runtimeDiagnostics,
    juce::AudioProcessorValueTreeState& parameters)
    : queue (sourceQueue),
      diagnostics (runtimeDiagnostics),
      heightParameter (parameters.getRawParameterValue (ParameterIDs::height)),
      curveParameter (parameters.getRawParameterValue (ParameterIDs::curve)),
      depthParameter (parameters.getRawParameterValue (ParameterIDs::depth)),
      tiltParameter (parameters.getRawParameterValue (ParameterIDs::tilt)),
      orbitParameter (parameters.getRawParameterValue (ParameterIDs::orbit)),
      zoomParameter (parameters.getRawParameterValue (ParameterIDs::zoom)),
      contrastParameter (parameters.getRawParameterValue (ParameterIDs::contrast)),
      smoothingParameter (parameters.getRawParameterValue (ParameterIDs::smooth)),
      historyParameter (parameters.getRawParameterValue (ParameterIDs::history)),
      resolutionParameter (parameters.getRawParameterValue (ParameterIDs::resolution)),
      rangeParameter (parameters.getRawParameterValue (ParameterIDs::range)),
      view2dParameter (parameters.getRawParameterValue (ParameterIDs::view2d)),
      repaintDriver ([this]
      {
          repaint();
          openGLContext.triggerRepaint();
      })
{
    for (auto* smoother : { &smoothedHeight, &smoothedLens, &smoothedDepth, &smoothedTilt,
                            &smoothedOrbit, &smoothedZoom, &smoothedContrast })
        smoother->reset (60.0, 0.12);

    const auto initial = sanitiseProjectionParameters ({
        heightParameter->load(), curveParameter->load(), depthParameter->load(),
        tiltParameter->load(), orbitParameter->load(), zoomParameter->load(),
        contrastParameter->load()
    });
    smoothedHeight.setCurrentAndTargetValue (initial.height);
    smoothedLens.setCurrentAndTargetValue (initial.lens);
    smoothedDepth.setCurrentAndTargetValue (initial.depth);
    smoothedTilt.setCurrentAndTargetValue (initial.tiltDegrees);
    smoothedOrbit.setCurrentAndTargetValue (initial.orbitDegrees);
    smoothedZoom.setCurrentAndTargetValue (initial.zoom);
    smoothedContrast.setCurrentAndTargetValue (initial.contrast);
    setOpaque (false);
    openGLContext.setOpenGLVersionRequired (juce::OpenGLContext::openGL3_2);
    openGLContext.setRenderer (this);
    openGLContext.setContinuousRepainting (false);
    openGLContext.setComponentPaintingEnabled (true);
    openGLContext.attachTo (*this);
    repaintDriver.start (60);
}

SpectralSurfaceComponent::~SpectralSurfaceComponent()
{
    repaintDriver.stop();
    openGLContext.detach();
}

void SpectralSurfaceComponent::paint (juce::Graphics& graphics)
{
    juce::String message;
    {
        const juce::ScopedLock lock (statusLock);
        message = statusText.isEmpty() ? "Starting 3D renderer..." : statusText;
    }
    graphics.setColour (juce::Colours::white.withAlpha (0.62f));
    graphics.setFont (14.0f);
    if (! rendererReady.load())
        graphics.drawFittedText (message, getLocalBounds().reduced (24),
                                 juce::Justification::centred, 6);

    graphics.setColour (juce::Colours::white.withAlpha (0.46f));
    graphics.setFont (11.0f);
    const auto count = std::clamp (guideLabelCount.load (std::memory_order_acquire), 0, 10);
    for (int index = 0; index < count; ++index)
    {
        const auto x = juce::roundToInt (guideLabelX[static_cast<std::size_t> (index)].load());
        const auto y = juce::roundToInt (guideLabelY[static_cast<std::size_t> (index)].load());
        graphics.drawText (formatFrequency (
                               guideLabelFrequency[static_cast<std::size_t> (index)].load()),
                           x - 62, y - 9, 58, 18, juce::Justification::centredRight);
    }
    graphics.drawText ("PAST", getLocalBounds().reduced (12, 8), juce::Justification::bottomLeft);
    graphics.drawText ("NOW  ->", getLocalBounds().reduced (52, 8), juce::Justification::bottomRight);
}

void SpectralSurfaceComponent::newOpenGLContextCreated()
{
    using namespace juce::gl;

    while (glGetError() != GL_NO_ERROR)
    {
    }

    diagnostics.setOpenGLInfo (
        juce::String (reinterpret_cast<const char*> (glGetString (GL_VENDOR)))
        + " | " + reinterpret_cast<const char*> (glGetString (GL_RENDERER))
        + " | " + reinterpret_cast<const char*> (glGetString (GL_VERSION)));

    auto candidate = std::make_unique<juce::OpenGLShaderProgram> (openGLContext);
    if (! candidate->addVertexShader (
            SpectralShaders::makeVertexShaderCompatible (
                juce::OpenGLHelpers::translateVertexShaderToV3 (SpectralShaders::vertex)))
        || ! candidate->addFragmentShader (
            juce::OpenGLHelpers::translateFragmentShaderToV3 (SpectralShaders::fragment))
        || ! candidate->link())
    {
        diagnostics.setShaderStatus (candidate->getLastError());
        diagnostics.shaderReady.store (false);
        setStatus ("3D renderer unavailable - audio is unaffected\n" + candidate->getLastError());
        return;
    }

    shader = std::move (candidate);
    auto guideCandidate = std::make_unique<juce::OpenGLShaderProgram> (openGLContext);
    if (! guideCandidate->addVertexShader (
            juce::OpenGLHelpers::translateVertexShaderToV3 (SpectralShaders::guideVertex))
        || ! guideCandidate->addFragmentShader (
            juce::OpenGLHelpers::translateFragmentShaderToV3 (SpectralShaders::guideFragment))
        || ! guideCandidate->link())
    {
        diagnostics.setShaderStatus (guideCandidate->getLastError());
        diagnostics.shaderReady.store (false);
        setStatus ("Frequency guide renderer unavailable - audio is unaffected\n"
                   + guideCandidate->getLastError());
        return;
    }
    guideShader = std::move (guideCandidate);
    meshDataAttribute = glGetAttribLocation (shader->getProgramID(), "meshData");
    textureUniform = glGetUniformLocation (shader->getProgramID(), "historyTexture");
    writeOffsetUniform = glGetUniformLocation (shader->getProgramID(), "writeOffset");
    heightScaleUniform = glGetUniformLocation (shader->getProgramID(), "heightScale");
    cameraMatrixUniform = glGetUniformLocation (shader->getProgramID(), "cameraMatrix");
    lensAmountUniform = glGetUniformLocation (shader->getProgramID(), "lensAmount");
    depthAmountUniform = glGetUniformLocation (shader->getProgramID(), "depthAmount");
    viewportAspectUniform = glGetUniformLocation (shader->getProgramID(), "viewportAspect");
    zoomAmountUniform = glGetUniformLocation (shader->getProgramID(), "zoomAmount");
    contrastAmountUniform = glGetUniformLocation (shader->getProgramID(), "contrastAmount");
    view2dAmountUniform = glGetUniformLocation (shader->getProgramID(), "view2dAmount");
    texelSizeUniform = glGetUniformLocation (shader->getProgramID(), "texelSize");
    frequencyScaleUniform = glGetUniformLocation (shader->getProgramID(), "frequencyScale");
    diagnostics.meshAttribute.store (meshDataAttribute);
    diagnostics.textureUniform.store (textureUniform);
    diagnostics.setShaderStatus ("linked");

    const auto projectionUniformMissing = heightScaleUniform < 0
                                       || cameraMatrixUniform < 0
                                       || lensAmountUniform < 0
                                       || depthAmountUniform < 0
                                       || viewportAspectUniform < 0
                                       || zoomAmountUniform < 0
                                       || contrastAmountUniform < 0
                                       || view2dAmountUniform < 0;
    if (projectionUniformMissing)
    {
        diagnostics.setShaderStatus ("missing projection uniform");
        diagnostics.shaderReady.store (false);
        setStatus ("3D renderer setup failed (missing projection uniform)");
        return;
    }

    guideDataAttribute = glGetAttribLocation (guideShader->getProgramID(), "guideData");
    guideCameraMatrixUniform = glGetUniformLocation (guideShader->getProgramID(), "cameraMatrix");
    guideLensAmountUniform = glGetUniformLocation (guideShader->getProgramID(), "lensAmount");
    guideDepthAmountUniform = glGetUniformLocation (guideShader->getProgramID(), "depthAmount");
    guideViewportAspectUniform = glGetUniformLocation (guideShader->getProgramID(), "viewportAspect");
    guideZoomAmountUniform = glGetUniformLocation (guideShader->getProgramID(), "zoomAmount");
    guideView2dAmountUniform = glGetUniformLocation (guideShader->getProgramID(), "view2dAmount");
    if (guideDataAttribute < 0 || guideCameraMatrixUniform < 0 || guideLensAmountUniform < 0
        || guideDepthAmountUniform < 0 || guideViewportAspectUniform < 0
        || guideZoomAmountUniform < 0 || guideView2dAmountUniform < 0)
    {
        diagnostics.setShaderStatus ("missing frequency guide uniform");
        diagnostics.shaderReady.store (false);
        setStatus ("3D renderer setup failed (missing frequency guide uniform)");
        return;
    }

    activeQuality = static_cast<AnalysisQuality> (std::clamp (
        juce::roundToInt (resolutionParameter->load()), 0, 2));
    activeRange = static_cast<FrequencyRange> (std::clamp (
        juce::roundToInt (rangeParameter->load()), 0, 3));
    const auto initialProfile = getQualityProfile (activeQuality);
    createSurfaceResources (surfaceResources, initialProfile.meshWidth, initialProfile.meshDepth);
    rebuildGuideResources();

    glGenTextures (1, &historyTexture);
    glBindTexture (GL_TEXTURE_2D, historyTexture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    clearTexture();

    frameAggregator.reset (static_cast<std::uint16_t> (initialProfile.bandCount));

    glEnable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
    if (const auto error = glGetError(); error != GL_NO_ERROR)
    {
        diagnostics.lastGlError.store (error);
        setStatus ("3D renderer setup failed (OpenGL error " + juce::String (error) + ")");
        return;
    }
    lastRenderMilliseconds = juce::Time::getMillisecondCounterHiRes();
    rendererReady.store (true);
    diagnostics.shaderReady.store (true);
    setStatus ({});
}

void SpectralSurfaceComponent::renderOpenGL()
{
    using namespace juce::gl;
    diagnostics.renderCalls.fetch_add (1, std::memory_order_relaxed);

    const auto scale = static_cast<float> (openGLContext.getRenderingScale());
    glViewport (0, 0,
                juce::roundToInt (scale * static_cast<float> (getWidth())),
                juce::roundToInt (scale * static_cast<float> (getHeight())));
    glClearColor (0.035f, 0.038f, 0.048f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (! rendererReady.load() || shader == nullptr)
        return;

    queue.serviceDiscardRequest();
    const auto requestedQuality = static_cast<AnalysisQuality> (std::clamp (
        juce::roundToInt (resolutionParameter->load()), 0, 2));
    const auto requestedRange = static_cast<FrequencyRange> (std::clamp (
        juce::roundToInt (rangeParameter->load()), 0, 3));
    if (requestedQuality != activeQuality || requestedRange != activeRange)
    {
        const auto qualityChanged = requestedQuality != activeQuality;
        activeQuality = requestedQuality;
        activeRange = requestedRange;
        const auto profile = getQualityProfile (activeQuality);
        if (qualityChanged)
        {
            deleteSurfaceResources (surfaceResources);
            createSurfaceResources (surfaceResources, profile.meshWidth, profile.meshDepth);
        }
        rebuildGuideResources();
        resetDisplayState (profile.bandCount);
        queue.requestDiscard();
        queue.serviceDiscardRequest();
    }

    if (clearRequested.exchange (false))
    {
        resetDisplayState (getQualityProfile (activeQuality).bandCount);
    }

    SpectrumFrame frame;
    const auto activeProfile = getQualityProfile (activeQuality);
    const auto expectedBandCount = static_cast<std::uint16_t> (activeProfile.bandCount);
    int poppedCount = 0;
    while (poppedCount < static_cast<int> (SpectrumFrameQueue::capacity) && queue.tryPop (frame))
    {
        ++poppedCount;
        if (! held.load() && frame.bandCount == expectedBandCount)
            frameAggregator.push (frame);
    }
    diagnostics.queuePops.fetch_add (static_cast<std::uint64_t> (poppedCount),
                                     std::memory_order_relaxed);

    if (! held.load())
    {
        rowProcessor.setSmoothing (smoothingParameter->load());
        cadence.setDuration (historyParameter->load());
        const auto now = juce::Time::getMillisecondCounterHiRes();
        const auto due = cadence.rowsDue ((now - lastRenderMilliseconds) / 1000.0);
        SpectrumFrame aggregate;
        if (due > 0 && frameAggregator.takePeakFrame (aggregate))
        {
            const auto smoothingElapsed = lastSmoothedFrameMilliseconds > 0.0
                ? (now - lastSmoothedFrameMilliseconds) / 1000.0
                : 0.0;
            const auto processed = rowProcessor.process (aggregate, smoothingElapsed);
            lastSmoothedFrameMilliseconds = now;
            for (int slot = 0; slot < due; ++slot)
                uploadFrame (processed);
        }
        lastRenderMilliseconds = now;
    }

    shader->use();
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, historyTexture);
    glUniform1i (textureUniform, 0);
    glUniform1f (writeOffsetUniform, static_cast<float> (writeRow) / 256.0f);

    const auto targets = sanitiseProjectionParameters ({
        heightParameter->load(), curveParameter->load(), depthParameter->load(),
        tiltParameter->load(), orbitParameter->load(), zoomParameter->load(),
        contrastParameter->load()
    });
    smoothedHeight.setTargetValue (targets.height);
    smoothedLens.setTargetValue (targets.lens);
    smoothedDepth.setTargetValue (targets.depth);
    smoothedTilt.setTargetValue (targets.tiltDegrees);
    smoothedOrbit.setTargetValue (targets.orbitDegrees);
    smoothedZoom.setTargetValue (targets.zoom);
    smoothedContrast.setTargetValue (targets.contrast);
    ProjectionParameters projection {
        smoothedHeight.getNextValue(), smoothedLens.getNextValue(),
        smoothedDepth.getNextValue(), smoothedTilt.getNextValue(),
        smoothedOrbit.getNextValue(), smoothedZoom.getNextValue(),
        smoothedContrast.getNextValue()
    };
    projection.view2d = view2dParameter->load() >= 0.5f;
    const auto viewportAspect = static_cast<float> (std::max (1, getWidth()))
                              / static_cast<float> (std::max (1, getHeight()));
    const auto cameraMatrix = makeCameraMatrix (projection, viewportAspect);
    glUniform1f (heightScaleUniform, projection.height);
    glUniformMatrix4fv (cameraMatrixUniform, 1, GL_FALSE, cameraMatrix.values.data());
    glUniform1f (lensAmountUniform, projection.lens);
    glUniform1f (depthAmountUniform, projection.depth);
    glUniform1f (viewportAspectUniform, viewportAspect);
    glUniform1f (zoomAmountUniform, projection.zoom);
    glUniform1f (contrastAmountUniform, projection.contrast);
    glUniform1f (view2dAmountUniform, projection.view2d ? 1.0f : 0.0f);
    diagnostics.setProjectionSnapshot (projection);
    glUniform1f (texelSizeUniform, 1.0f / static_cast<float> (surfaceResources.meshWidth - 1));
    glUniform1f (frequencyScaleUniform, static_cast<float> (activeProfile.bandCount) / 1024.0f);

    glBindVertexArray (surfaceResources.vertexArray);
    glDrawElements (GL_TRIANGLES, surfaceResources.indexCount, GL_UNSIGNED_INT, nullptr);

    guideShader->use();
    glUniformMatrix4fv (guideCameraMatrixUniform, 1, GL_FALSE, cameraMatrix.values.data());
    glUniform1f (guideLensAmountUniform, projection.lens);
    glUniform1f (guideDepthAmountUniform, projection.depth);
    glUniform1f (guideViewportAspectUniform, viewportAspect);
    glUniform1f (guideZoomAmountUniform, projection.zoom);
    glUniform1f (guideView2dAmountUniform, projection.view2d ? 1.0f : 0.0f);
    glDisable (GL_DEPTH_TEST);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE);
    glBindVertexArray (guideVertexArray);
    glDrawArrays (GL_LINES, 0, guideVertexCount);
    glDisable (GL_BLEND);
    glEnable (GL_DEPTH_TEST);
    updateGuideLabels (projection, viewportAspect);
    diagnostics.setDisplaySnapshot (
        projection.view2d, static_cast<int> (activeQuality), static_cast<int> (activeRange),
        activeProfile.bandCount, static_cast<int> (activeGuides.count),
        static_cast<float> (rowProcessor.getTimeConstantSeconds()));
    if (const auto error = glGetError(); error != GL_NO_ERROR)
        diagnostics.lastGlError.store (error);
}

void SpectralSurfaceComponent::openGLContextClosing()
{
    using namespace juce::gl;
    rendererReady.store (false);
    diagnostics.shaderReady.store (false);
    shader.reset();
    guideShader.reset();
    if (historyTexture != 0) glDeleteTextures (1, &historyTexture);
    deleteSurfaceResources (surfaceResources);
    deleteGuideResources();
    historyTexture = 0;
}

void SpectralSurfaceComponent::uploadFrame (const SpectrumFrame& frame)
{
    using namespace juce::gl;
    diagnostics.rowsUploaded.fetch_add (1, std::memory_order_relaxed);
    diagnostics.recordUploadedPeak (*std::max_element (frame.magnitudes.begin(),
                                                       frame.magnitudes.begin() + frame.bandCount));
    glBindTexture (GL_TEXTURE_2D, historyTexture);
    glTexSubImage2D (GL_TEXTURE_2D, 0, 0, writeRow, frame.bandCount, 1,
                     GL_RED, GL_FLOAT, frame.magnitudes.data());
    writeRow = (writeRow + 1) % 256;
}

void SpectralSurfaceComponent::clearTexture()
{
    using namespace juce::gl;
    glBindTexture (GL_TEXTURE_2D, historyTexture);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_R16F, 1024, 256, 0,
                  GL_RED, GL_FLOAT, clearPixels.data());
}

void SpectralSurfaceComponent::createSurfaceResources (SurfaceResources& resources,
                                                       int width,
                                                       int depth)
{
    using namespace juce::gl;
    const SurfaceMesh mesh (width, depth);
    resources.meshWidth = width;
    resources.meshDepth = depth;
    resources.indexCount = static_cast<int> (mesh.indices.size());
    resources.payloadBytes = mesh.payloadBytes();

    glGenVertexArrays (1, &resources.vertexArray);
    glBindVertexArray (resources.vertexArray);
    glGenBuffers (1, &resources.vertexBuffer);
    glBindBuffer (GL_ARRAY_BUFFER, resources.vertexBuffer);
    glBufferData (GL_ARRAY_BUFFER,
                  static_cast<GLsizeiptr> (mesh.vertices.size() * sizeof (SurfaceVertex)),
                  mesh.vertices.data(),
                  GL_STATIC_DRAW);
    glGenBuffers (1, &resources.indexBuffer);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, resources.indexBuffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER,
                  static_cast<GLsizeiptr> (mesh.indices.size() * sizeof (std::uint32_t)),
                  mesh.indices.data(),
                  GL_STATIC_DRAW);
    glEnableVertexAttribArray (static_cast<GLuint> (meshDataAttribute));
    glVertexAttribPointer (static_cast<GLuint> (meshDataAttribute),
                           4, GL_FLOAT, GL_FALSE, sizeof (SurfaceVertex), nullptr);
}

void SpectralSurfaceComponent::deleteSurfaceResources (SurfaceResources& resources)
{
    using namespace juce::gl;
    if (resources.vertexBuffer != 0) glDeleteBuffers (1, &resources.vertexBuffer);
    if (resources.indexBuffer != 0) glDeleteBuffers (1, &resources.indexBuffer);
    if (resources.vertexArray != 0) glDeleteVertexArrays (1, &resources.vertexArray);
    resources = {};
}

void SpectralSurfaceComponent::rebuildGuideResources()
{
    using namespace juce::gl;
    deleteGuideResources();
    activeGuides = makeFrequencyGuides (getFrequencyRangeProfile (activeRange),
                                        diagnostics.analysisUpperHz.load (std::memory_order_relaxed));
    std::array<float, 40> vertices {};
    for (std::size_t index = 0; index < activeGuides.count; ++index)
    {
        const auto offset = index * 4;
        vertices[offset] = -1.0f;
        vertices[offset + 1] = activeGuides.values[index].normalisedFrequency;
        vertices[offset + 2] = 1.0f;
        vertices[offset + 3] = activeGuides.values[index].normalisedFrequency;
    }
    guideVertexCount = static_cast<int> (activeGuides.count * 2);
    glGenVertexArrays (1, &guideVertexArray);
    glBindVertexArray (guideVertexArray);
    glGenBuffers (1, &guideVertexBuffer);
    glBindBuffer (GL_ARRAY_BUFFER, guideVertexBuffer);
    glBufferData (GL_ARRAY_BUFFER,
                  static_cast<GLsizeiptr> (activeGuides.count * 4 * sizeof (float)),
                  vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray (static_cast<GLuint> (guideDataAttribute));
    glVertexAttribPointer (static_cast<GLuint> (guideDataAttribute), 2, GL_FLOAT, GL_FALSE,
                           2 * sizeof (float), nullptr);
}

void SpectralSurfaceComponent::deleteGuideResources()
{
    using namespace juce::gl;
    if (guideVertexBuffer != 0) glDeleteBuffers (1, &guideVertexBuffer);
    if (guideVertexArray != 0) glDeleteVertexArrays (1, &guideVertexArray);
    guideVertexBuffer = 0;
    guideVertexArray = 0;
    guideVertexCount = 0;
}

void SpectralSurfaceComponent::resetDisplayState (int bandCount)
{
    clearTexture();
    rowProcessor.reset();
    lastSmoothedFrameMilliseconds = 0.0;
    frameAggregator.reset (static_cast<std::uint16_t> (bandCount));
    cadence.reset();
    writeRow = 0;
}

void SpectralSurfaceComponent::updateGuideLabels (const ProjectionParameters& projection,
                                                  float aspect)
{
    for (std::size_t index = 0; index < activeGuides.count; ++index)
    {
        const auto point = projectSurfacePoint (0.92f,
                                                activeGuides.values[index].normalisedFrequency,
                                                0.0f, aspect, projection);
        guideLabelX[index].store ((point.x * 0.5f + 0.5f) * static_cast<float> (getWidth()),
                                  std::memory_order_relaxed);
        guideLabelY[index].store ((0.5f - point.y * 0.5f) * static_cast<float> (getHeight()),
                                  std::memory_order_relaxed);
        guideLabelFrequency[index].store (
            juce::roundToInt (activeGuides.values[index].frequencyHz),
            std::memory_order_relaxed);
    }
    guideLabelCount.store (static_cast<int> (activeGuides.count), std::memory_order_release);
}

void SpectralSurfaceComponent::setStatus (juce::String message)
{
    const juce::ScopedLock lock (statusLock);
    statusText = std::move (message);
}
