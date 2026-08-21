#pragma once

#include <juce_core/juce_core.h>

namespace SpectralShaders
{
inline constexpr auto vertex = R"GLSL(
attribute vec4 meshData;
uniform sampler2D historyTexture;
uniform float writeOffset;
uniform float heightScale;
uniform mat4 cameraMatrix;
uniform float lensAmount;
uniform float depthAmount;
uniform float viewportAspect;
uniform float zoomAmount;
uniform float view2dAmount;
uniform float texelSize;
uniform float frequencyScale;
varying float magnitude;
varying float lighting;
varying float age;

float sampleHeight(vec2 uv)
{
    return texture2D(historyTexture, vec2(uv.x * frequencyScale,
                                          fract(uv.y + writeOffset))).r;
}

void main()
{
    vec2 uv = meshData.zw;
    magnitude = sampleHeight(uv);
    float leftHeight = sampleHeight(uv - vec2(texelSize, 0.0));
    float rightHeight = sampleHeight(uv + vec2(texelSize, 0.0));
    float oldHeight = sampleHeight(uv - vec2(0.0, texelSize));
    float newHeight = sampleHeight(uv + vec2(0.0, texelSize));
    vec3 normal = normalize(vec3((leftHeight - rightHeight) * heightScale,
                                 2.0 * texelSize,
                                 (oldHeight - newHeight) * heightScale));
    lighting = 0.58 + 0.42 * max(dot(normalize(vec3(-0.35, 0.82, 0.45)), normal), 0.0);
    age = uv.y;

    if (view2dAmount > 0.5)
    {
        lighting = 1.0;
        gl_Position = vec4(meshData.y, meshData.x, 0.0, 1.0);
        return;
    }

    vec4 world = vec4(meshData.y * depthAmount,
                      meshData.x,
                      magnitude * heightScale * 0.34,
                      1.0);
    vec4 clip = cameraMatrix * world;
    vec2 screen = clip.xy / max(clip.w, 0.0001);
    vec2 radial = vec2(screen.x * viewportAspect, screen.y);
    float radius2 = dot(radial, radial);
    radial *= inversesqrt(max(1.0 + lensAmount * radius2, 0.1));
    screen = vec2(radial.x / viewportAspect, radial.y) * zoomAmount;
    gl_Position = vec4(screen * clip.w, clip.z, clip.w);
}
)GLSL";

inline juce::String makeVertexShaderCompatible (juce::String translatedShader)
{
    if (translatedShader.startsWith ("#version 150"))
        return translatedShader.replace ("texture2D", "texture");

    return translatedShader;
}

inline constexpr auto fragment = R"GLSL(
varying float magnitude;
varying float lighting;
varying float age;
uniform float contrastAmount;

vec3 spectralColour(float value)
{
    vec3 purple = vec3(0.20, 0.035, 0.34);
    vec3 blue = vec3(0.035, 0.20, 0.72);
    vec3 cyan = vec3(0.00, 0.68, 0.78);
    vec3 green = vec3(0.18, 0.84, 0.42);
    vec3 yellow = vec3(0.90, 0.91, 0.15);
    vec3 orange = vec3(1.00, 0.30, 0.08);
    if (value < 0.22) return mix(purple, blue, value / 0.22);
    if (value < 0.45) return mix(blue, cyan, (value - 0.22) / 0.23);
    if (value < 0.67) return mix(cyan, green, (value - 0.45) / 0.22);
    if (value < 0.86) return mix(green, yellow, (value - 0.67) / 0.19);
    return mix(yellow, orange, (value - 0.86) / 0.14);
}

void main()
{
    vec3 background = vec3(0.035, 0.038, 0.048);
    float displayMagnitude = clamp(0.5 + (magnitude - 0.5) * contrastAmount, 0.0, 1.0);
    float ridgeLight = smoothstep(0.015, 0.09, fwidth(displayMagnitude));
    float isoDistance = abs(fract(displayMagnitude * 8.0) - 0.5);
    float contourLight = 1.0 - smoothstep(0.42, 0.49, isoDistance);
    float displayLight = 0.32 + 0.68 * displayMagnitude;
    vec3 colour = spectralColour(displayMagnitude)
                * lighting * displayLight
                + vec3(0.16, 0.30, 0.34) * ridgeLight
                + vec3(0.06, 0.10, 0.12) * contourLight;
    float signal = smoothstep(0.015, 0.12, displayMagnitude);
    float oldEdge = smoothstep(0.0, 0.12, age);
    gl_FragColor = vec4(mix(background, colour, signal * oldEdge), 1.0);
}
)GLSL";

inline constexpr auto guideVertex = R"GLSL(
attribute vec2 guideData;
uniform mat4 cameraMatrix;
uniform float lensAmount;
uniform float depthAmount;
uniform float viewportAspect;
uniform float zoomAmount;
uniform float view2dAmount;

void main()
{
    if (view2dAmount > 0.5)
    {
        gl_Position = vec4(guideData.x, guideData.y, 0.0, 1.0);
        return;
    }
    vec4 world = vec4(guideData.x * depthAmount, guideData.y, 0.025, 1.0);
    vec4 clip = cameraMatrix * world;
    vec2 screen = clip.xy / max(clip.w, 0.0001);
    vec2 radial = vec2(screen.x * viewportAspect, screen.y);
    float radius2 = dot(radial, radial);
    radial *= inversesqrt(max(1.0 + lensAmount * radius2, 0.1));
    screen = vec2(radial.x / viewportAspect, radial.y) * zoomAmount;
    gl_Position = vec4(screen * clip.w, clip.z, clip.w);
}
)GLSL";

inline constexpr auto guideFragment = R"GLSL(
void main()
{
    gl_FragColor = vec4(0.48, 0.92, 0.88, 0.58);
}
)GLSL";
} // namespace SpectralShaders
