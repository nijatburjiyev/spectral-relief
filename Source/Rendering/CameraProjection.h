#pragma once

#include <array>

struct ProjectionParameters
{
    float height = 1.4f;
    float lens = 0.35f;
    float depth = 1.0f;
    float tiltDegrees = 55.0f;
    float orbitDegrees = 0.0f;
    float zoom = 1.0f;
    float contrast = 1.0f;
    bool view2d = false;
};

struct LensPoint
{
    float x;
    float y;
};

struct WorldPoint
{
    float x;
    float y;
    float z;
};

struct ProjectedSurfacePoint
{
    float x;
    float y;
    float depth;
};

struct CameraMatrix
{
    std::array<float, 16> values;
};

[[nodiscard]] ProjectionParameters sanitiseProjectionParameters (ProjectionParameters) noexcept;
[[nodiscard]] WorldPoint makeWorldPoint (float time,
                                         float frequency,
                                         float magnitude,
                                         const ProjectionParameters&) noexcept;
[[nodiscard]] CameraMatrix makeCameraMatrix (const ProjectionParameters&, float aspect) noexcept;
[[nodiscard]] LensPoint projectLensPoint (LensPoint, float aspect, float lens) noexcept;
[[nodiscard]] float projectedRadius (LensPoint) noexcept;
[[nodiscard]] ProjectedSurfacePoint projectSurfacePoint (float time,
                                                         float frequency,
                                                         float magnitude,
                                                         float aspect,
                                                         const ProjectionParameters&) noexcept;
