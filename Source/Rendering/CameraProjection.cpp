#include "CameraProjection.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float pi = 3.14159265358979323846f;

struct Vec3
{
    float x;
    float y;
    float z;
};

[[nodiscard]] float finiteOr (float value, float fallback) noexcept
{
    return std::isfinite (value) ? value : fallback;
}

[[nodiscard]] Vec3 subtract (Vec3 lhs, Vec3 rhs) noexcept
{
    return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

[[nodiscard]] Vec3 cross (Vec3 lhs, Vec3 rhs) noexcept
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

[[nodiscard]] float dot (Vec3 lhs, Vec3 rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] Vec3 normalise (Vec3 vector) noexcept
{
    const auto lengthSquared = std::max (dot (vector, vector), 1.0e-12f);
    const auto inverseLength = 1.0f / std::sqrt (lengthSquared);
    return { vector.x * inverseLength, vector.y * inverseLength, vector.z * inverseLength };
}

[[nodiscard]] CameraMatrix multiply (const CameraMatrix& lhs, const CameraMatrix& rhs) noexcept
{
    CameraMatrix result {};
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            for (int index = 0; index < 4; ++index)
                result.values[static_cast<std::size_t> (column * 4 + row)]
                    += lhs.values[static_cast<std::size_t> (index * 4 + row)]
                     * rhs.values[static_cast<std::size_t> (column * 4 + index)];
    return result;
}

struct ClipPoint
{
    float x;
    float y;
    float z;
    float w;
};

[[nodiscard]] ClipPoint transform (const CameraMatrix& matrix, WorldPoint point) noexcept
{
    const auto& m = matrix.values;
    return {
        m[0] * point.x + m[4] * point.y + m[8] * point.z + m[12],
        m[1] * point.x + m[5] * point.y + m[9] * point.z + m[13],
        m[2] * point.x + m[6] * point.y + m[10] * point.z + m[14],
        m[3] * point.x + m[7] * point.y + m[11] * point.z + m[15]
    };
}
} // namespace

ProjectionParameters sanitiseProjectionParameters (ProjectionParameters parameters) noexcept
{
    const ProjectionParameters defaults;
    parameters.height = std::clamp (finiteOr (parameters.height, defaults.height), 0.25f, 3.0f);
    parameters.lens = std::clamp (finiteOr (parameters.lens, defaults.lens), 0.0f, 1.0f);
    parameters.depth = std::clamp (finiteOr (parameters.depth, defaults.depth), 0.5f, 2.0f);
    parameters.tiltDegrees = std::clamp (finiteOr (parameters.tiltDegrees, defaults.tiltDegrees), 15.0f, 90.0f);
    parameters.orbitDegrees = std::clamp (finiteOr (parameters.orbitDegrees, defaults.orbitDegrees), -180.0f, 180.0f);
    parameters.zoom = std::clamp (finiteOr (parameters.zoom, defaults.zoom), 0.65f, 1.6f);
    parameters.contrast = std::clamp (finiteOr (parameters.contrast, defaults.contrast), 0.5f, 2.5f);
    return parameters;
}

WorldPoint makeWorldPoint (float time,
                           float frequency,
                           float magnitude,
                           const ProjectionParameters& parameters) noexcept
{
    const auto safe = sanitiseProjectionParameters (parameters);
    return {
        finiteOr (time, 0.0f) * safe.depth,
        finiteOr (frequency, 0.0f),
        finiteOr (magnitude, 0.0f) * safe.height
    };
}

CameraMatrix makeCameraMatrix (const ProjectionParameters& parameters, float aspect) noexcept
{
    const auto safe = sanitiseProjectionParameters (parameters);
    const auto safeAspect = std::clamp (finiteOr (aspect, 1.0f), 0.25f, 4.0f);
    const auto tilt = safe.tiltDegrees * pi / 180.0f;
    const auto orbit = safe.orbitDegrees * pi / 180.0f;
    const auto azimuth = orbit - 0.5f * pi;
    constexpr float radius = 4.0f;
    const Vec3 eye {
        radius * std::cos (tilt) * std::cos (azimuth),
        radius * std::cos (tilt) * std::sin (azimuth),
        radius * std::sin (tilt)
    };
    const Vec3 forward = normalise (subtract ({ 0.0f, 0.0f, 0.0f }, eye));
    const Vec3 side { std::cos (orbit), std::sin (orbit), 0.0f };
    const Vec3 up = cross (side, forward);

    const CameraMatrix view { {
        side.x, up.x, -forward.x, 0.0f,
        side.y, up.y, -forward.y, 0.0f,
        side.z, up.z, -forward.z, 0.0f,
        -dot (side, eye), -dot (up, eye), dot (forward, eye), 1.0f
    } };

    constexpr float nearPlane = 0.1f;
    constexpr float farPlane = 20.0f;
    const auto focalLength = 1.0f / std::tan (48.0f * pi / 360.0f);
    const auto depthScale = (farPlane + nearPlane) / (nearPlane - farPlane);
    const auto depthOffset = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    const CameraMatrix perspective { {
        focalLength / safeAspect, 0.0f, 0.0f, 0.0f,
        0.0f, focalLength, 0.0f, 0.0f,
        0.0f, 0.0f, depthScale, -1.0f,
        0.0f, 0.0f, depthOffset, 0.0f
    } };

    return multiply (perspective, view);
}

LensPoint projectLensPoint (LensPoint point, float aspect, float lens) noexcept
{
    const auto safeAspect = std::clamp (finiteOr (aspect, 1.0f), 0.25f, 4.0f);
    const auto safeLens = std::clamp (finiteOr (lens, 0.35f), 0.0f, 1.0f);
    const LensPoint safePoint { finiteOr (point.x, 0.0f), finiteOr (point.y, 0.0f) };
    const LensPoint aspectPoint { safePoint.x * safeAspect, safePoint.y };
    const auto radius2 = aspectPoint.x * aspectPoint.x + aspectPoint.y * aspectPoint.y;
    const auto scale = 1.0f / std::sqrt (1.0f + safeLens * radius2);
    return { aspectPoint.x * scale / safeAspect, aspectPoint.y * scale };
}

float projectedRadius (LensPoint point) noexcept
{
    const auto x = finiteOr (point.x, 0.0f);
    const auto y = finiteOr (point.y, 0.0f);
    return std::sqrt (x * x + y * y);
}

ProjectedSurfacePoint projectSurfacePoint (float time,
                                           float frequency,
                                           float magnitude,
                                           float aspect,
                                           const ProjectionParameters& parameters) noexcept
{
    const auto safe = sanitiseProjectionParameters (parameters);
    if (safe.view2d)
        return { finiteOr (time, 0.0f), finiteOr (frequency, 0.0f), 0.0f };

    const auto clip = transform (makeCameraMatrix (safe, aspect),
                                 makeWorldPoint (time, frequency, magnitude, safe));
    const auto safeW = std::abs (clip.w) < 1.0e-6f
        ? std::copysign (1.0e-6f, clip.w == 0.0f ? 1.0f : clip.w)
        : clip.w;
    const auto lensPoint = projectLensPoint ({ clip.x / safeW, clip.y / safeW }, aspect, safe.lens);
    return { lensPoint.x * safe.zoom, lensPoint.y * safe.zoom, clip.z / safeW };
}
