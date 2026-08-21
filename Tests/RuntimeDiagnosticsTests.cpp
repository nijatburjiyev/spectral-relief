#include "Source/Diagnostics/RuntimeDiagnostics.h"

#include <juce_core/juce_core.h>

class RuntimeDiagnosticsTests final : public juce::UnitTest
{
public:
    RuntimeDiagnosticsTests() : juce::UnitTest ("Runtime diagnostics") {}

    void runTest() override
    {
        beginTest ("snapshot reports every realtime pipeline stage");

        RuntimeDiagnostics diagnostics;
        diagnostics.processBlocks.store (12);
        diagnostics.processedSamples.store (4096);
        diagnostics.inputPeak.store (0.75f);
        diagnostics.fftFrames.store (8);
        diagnostics.queuePushes.store (7);
        diagnostics.queueDrops.store (1);
        diagnostics.renderCalls.store (30);
        diagnostics.queuePops.store (7);
        diagnostics.rowsUploaded.store (6);
        diagnostics.uploadedPeak.store (0.62f);
        diagnostics.lastGlError.store (1282);
        diagnostics.shaderReady.store (true);
        ProjectionParameters projection;
        projection.height = 2.25f;
        projection.lens = 0.7f;
        projection.depth = 1.6f;
        projection.tiltDegrees = 62.0f;
        projection.orbitDegrees = -24.0f;
        projection.zoom = 1.3f;
        projection.contrast = 1.8f;
        diagnostics.setProjectionSnapshot (projection);
        diagnostics.setDisplaySnapshot (true, 2, 3, 1024, 4, 1.5f);
        diagnostics.setAverageSnapshot (80.0f, 0.020f, 0.080f);

        const auto line = diagnostics.snapshotLine();
        expect (line.contains ("blocks=12"));
        expect (line.contains ("samples=4096"));
        expect (line.contains ("inputPeak=0.750"));
        expect (line.contains ("fft=8"));
        expect (line.contains ("push=7"));
        expect (line.contains ("drop=1"));
        expect (line.contains ("render=30"));
        expect (line.contains ("pop=7"));
        expect (line.contains ("upload=6"));
        expect (line.contains ("uploadPeak=0.620"));
        expect (line.contains ("shader=1"));
        expect (line.contains ("glError=1282"));
        expect (line.contains ("height=2.250"));
        expect (line.contains ("lens=0.700"));
        expect (line.contains ("depth=1.600"));
        expect (line.contains ("tilt=62.000"));
        expect (line.contains ("orbit=-24.000"));
        expect (line.contains ("zoom=1.300"));
        expect (line.contains ("contrast=1.800"));
        expect (line.contains ("view=2D"));
        expect (line.contains ("resolution=2"));
        expect (line.contains ("range=3"));
        expect (line.contains ("bands=1024"));
        expect (line.contains ("guides=4"));
        expect (line.contains ("smoothTau=1.500"));
        expect (line.contains ("averageMs=80.0"));
        expect (line.contains ("averageAttack=0.020"));
        expect (line.contains ("averageRelease=0.080"));
    }
};

static RuntimeDiagnosticsTests runtimeDiagnosticsTests;
