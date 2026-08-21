#include "Source/Rendering/CameraProjection.h"

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>

namespace
{
class CameraProjectionTests final : public juce::UnitTest
{
public:
    CameraProjectionTests() : juce::UnitTest ("Camera projection", "rendering") {}

    void runTest() override
    {
        beginTest ("zero lens is an exact identity with a stable centre");
        const auto identity = projectLensPoint ({ 0.7f, -0.2f }, 16.0f / 9.0f, 0.0f);
        expectWithinAbsoluteError (identity.x, 0.7f, 1.0e-6f);
        expectWithinAbsoluteError (identity.y, -0.2f, 1.0e-6f);
        const auto opticalCentre = projectLensPoint ({ 0.0f, 0.0f }, 16.0f / 9.0f, 1.0f);
        expectWithinAbsoluteError (opticalCentre.x, 0.0f, 1.0e-6f);
        expectWithinAbsoluteError (opticalCentre.y, 0.0f, 1.0e-6f);

        ProjectionParameters flat;
        flat.lens = 0.0f;
        const auto centre = projectSurfacePoint (0.0f, 0.0f, 0.0f, 16.0f / 9.0f, flat);
        expectWithinAbsoluteError (centre.x, 0.0f, 1.0e-6f);

        beginTest ("radial lens is symmetric and monotonically compresses the edges");
        const auto right = projectLensPoint ({ 0.7f, 0.2f }, 16.0f / 9.0f, 1.0f);
        const auto left = projectLensPoint ({ -0.7f, -0.2f }, 16.0f / 9.0f, 1.0f);
        expectWithinAbsoluteError (left.x, -right.x, 1.0e-6f);
        expectWithinAbsoluteError (left.y, -right.y, 1.0e-6f);

        const auto nearRadius = projectedRadius (projectLensPoint ({ 0.2f, 0.0f }, 1.0f, 1.0f));
        const auto farRadius = projectedRadius (projectLensPoint ({ 0.8f, 0.0f }, 1.0f, 1.0f));
        expect (nearRadius < farRadius);
        expect (farRadius < 0.8f);

        beginTest ("height and depth control independent world axes");
        auto high = flat;
        high.height = 3.0f;
        const auto flatWorld = makeWorldPoint (0.4f, -0.2f, 0.8f, flat);
        const auto highWorld = makeWorldPoint (0.4f, -0.2f, 0.8f, high);
        expectWithinAbsoluteError (flatWorld.x, highWorld.x, 1.0e-6f);
        expectWithinAbsoluteError (flatWorld.y, highWorld.y, 1.0e-6f);
        expect (highWorld.z > flatWorld.z);

        auto deep = flat;
        deep.depth = 2.0f;
        const auto deepWorld = makeWorldPoint (0.4f, -0.2f, 0.8f, deep);
        expect (deepWorld.x > flatWorld.x);
        expectWithinAbsoluteError (deepWorld.y, flatWorld.y, 1.0e-6f);
        expectWithinAbsoluteError (deepWorld.z, flatWorld.z, 1.0e-6f);

        beginTest ("contrast never changes surface geometry");
        auto contrasted = flat;
        contrasted.contrast = 2.5f;
        const auto normalProjection = projectSurfacePoint (0.4f, -0.2f, 0.8f, 1.6f, flat);
        const auto contrastProjection = projectSurfacePoint (0.4f, -0.2f, 0.8f, 1.6f, contrasted);
        expectWithinAbsoluteError (normalProjection.x, contrastProjection.x, 1.0e-6f);
        expectWithinAbsoluteError (normalProjection.y, contrastProjection.y, 1.0e-6f);
        expectWithinAbsoluteError (normalProjection.depth, contrastProjection.depth, 1.0e-6f);

        beginTest ("2D view exactly bypasses every 3D deformation control");
        ProjectionParameters view2d;
        view2d.view2d = true;
        view2d.height = 3.0f;
        view2d.lens = 1.0f;
        view2d.depth = 2.0f;
        view2d.tiltDegrees = 15.0f;
        view2d.orbitDegrees = 180.0f;
        view2d.zoom = 1.6f;
        const auto flat2d = projectSurfacePoint (0.63f, -0.42f, 1.0f, 3.0f, view2d);
        expectWithinAbsoluteError (flat2d.x, 0.63f, 1.0e-6f);
        expectWithinAbsoluteError (flat2d.y, -0.42f, 1.0e-6f);
        expectWithinAbsoluteError (flat2d.depth, 0.0f, 1.0e-6f);

        beginTest ("exact top view keeps time right and frequency up");
        ProjectionParameters top;
        top.tiltDegrees = 90.0f;
        top.orbitDegrees = 0.0f;
        top.lens = 0.0f;
        const auto topCentre = projectSurfacePoint (0.0f, 0.0f, 0.0f, 1.0f, top);
        const auto later = projectSurfacePoint (0.5f, 0.0f, 0.0f, 1.0f, top);
        const auto higher = projectSurfacePoint (0.0f, 0.5f, 0.0f, 1.0f, top);
        expect (later.x > topCentre.x);
        expectWithinAbsoluteError (later.y, topCentre.y, 1.0e-5f);
        expect (higher.y > topCentre.y);
        expectWithinAbsoluteError (higher.x, topCentre.x, 1.0e-5f);

        beginTest ("top camera remains finite through a complete orbit");
        for (const auto orbit : { -180.0f, -90.0f, 0.0f, 90.0f, 180.0f })
        {
            top.orbitDegrees = orbit;
            const auto matrix = makeCameraMatrix (top, 16.0f / 9.0f);
            for (const auto value : matrix.values)
                expect (std::isfinite (value));
        }

        beginTest ("all approved parameter corners produce finite projections");
        constexpr std::array<float, 2> heights { 0.25f, 3.0f };
        constexpr std::array<float, 2> lenses { 0.0f, 1.0f };
        constexpr std::array<float, 2> depths { 0.5f, 2.0f };
        constexpr std::array<float, 2> tilts { 15.0f, 85.0f };
        constexpr std::array<float, 2> orbits { -60.0f, 60.0f };
        constexpr std::array<float, 2> zooms { 0.65f, 1.6f };
        constexpr std::array<float, 2> contrasts { 0.5f, 2.5f };
        constexpr std::array<float, 3> aspects { 0.5f, 1.0f, 3.0f };

        for (const auto height : heights)
            for (const auto lens : lenses)
                for (const auto depth : depths)
                    for (const auto tilt : tilts)
                        for (const auto orbit : orbits)
                            for (const auto zoom : zooms)
                                for (const auto contrast : contrasts)
                                    for (const auto aspect : aspects)
                                    {
                                        ProjectionParameters parameters {
                                            height, lens, depth, tilt, orbit, zoom, contrast
                                        };
                                        const auto matrix = makeCameraMatrix (parameters, aspect);
                                        for (const auto value : matrix.values)
                                            expect (std::isfinite (value));

                                        const auto point = projectSurfacePoint (
                                            0.9f, -0.8f, 1.0f, aspect, parameters);
                                        expect (std::isfinite (point.x));
                                        expect (std::isfinite (point.y));
                                        expect (std::isfinite (point.depth));
                                    }
    }
};

CameraProjectionTests cameraProjectionTests;
} // namespace
