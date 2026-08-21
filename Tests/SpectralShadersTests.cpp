#include "Source/Rendering/SpectralShaders.h"

#include <juce_core/juce_core.h>

namespace
{
class SpectralShadersTests final : public juce::UnitTest
{
public:
    SpectralShadersTests() : juce::UnitTest ("Spectral shaders", "rendering") {}

    void runTest() override
    {
        beginTest ("modern vertex shader uses the GLSL 1.50 texture function");
        const auto shader = SpectralShaders::makeVertexShaderCompatible (
            juce::String ("#version 150\n") + SpectralShaders::vertex);
        expect (shader.startsWith ("#version 150"));
        expect (shader.contains ("texture("));
        expect (! shader.contains ("texture2D("));
        expect (shader.contains ("uniform float frequencyScale"));
        for (const auto* token : { "uniform mat4 cameraMatrix", "uniform float lensAmount",
                                   "uniform float depthAmount", "uniform float viewportAspect",
                                   "uniform float zoomAmount", "uniform float contrastAmount",
                                   "radius2", "inversesqrt" })
            expect (shader.contains (token)
                    || juce::String (SpectralShaders::fragment).contains (token));
        expect (! shader.contains ("curveAngle"));
        expect (shader.contains ("uniform float view2dAmount"));
        expect (shader.contains ("if (view2dAmount > 0.5)"));
        const auto fragment = juce::String (SpectralShaders::fragment);
        expect (fragment.contains ("0.5 + (magnitude - 0.5) * contrastAmount"));
        expect (! juce::String (SpectralShaders::vertex).contains ("contrastAmount"));
        expect (juce::String (SpectralShaders::guideVertex).contains ("guideData"));

        beginTest ("legacy vertex shader keeps the GLSL 1.10 texture function");
        const auto legacyShader = SpectralShaders::makeVertexShaderCompatible (
            juce::String (SpectralShaders::vertex));
        expect (legacyShader.contains ("texture2D("));
    }
};

SpectralShadersTests spectralShadersTests;
} // namespace
