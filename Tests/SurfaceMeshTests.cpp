#include "Source/Rendering/SurfaceMesh.h"

#include <juce_core/juce_core.h>

#include <algorithm>

namespace
{
class SurfaceMeshTests final : public juce::UnitTest
{
public:
    SurfaceMeshTests() : juce::UnitTest ("Surface mesh", "rendering") {}

    void runTest() override
    {
        const SurfaceMesh normal (256, 256);
        const SurfaceMesh high (512, 256);

        beginTest ("mesh has one vertex per texture sample");
        expectEquals (static_cast<int> (normal.vertices.size()), 256 * 256);
        expectEquals (static_cast<int> (high.vertices.size()), 512 * 256);
        expectWithinAbsoluteError (normal.vertices.front().u, 0.0f, 1.0e-7f);
        expectWithinAbsoluteError (normal.vertices.front().v, 0.0f, 1.0e-7f);
        expectWithinAbsoluteError (high.vertices.back().u, 1.0f, 1.0e-7f);
        expectWithinAbsoluteError (high.vertices.back().v, 1.0f, 1.0e-7f);

        beginTest ("meshes use consistently-wound 32-bit indices");
        expectEquals (static_cast<int> (normal.indices.size()), 255 * 255 * 6);
        expectEquals (static_cast<int> (high.indices.size()), 511 * 255 * 6);
        expect (high.uses32BitIndices());
        expectEquals (static_cast<int> (normal.indices[0]), 0);
        expectEquals (static_cast<int> (normal.indices[1]), 1);
        expectEquals (static_cast<int> (normal.indices[2]), 257);
        expectEquals (static_cast<int> (normal.indices[3]), 0);
        expectEquals (static_cast<int> (normal.indices[4]), 257);
        expectEquals (static_cast<int> (normal.indices[5]), 256);

        beginTest ("active Ultra mesh and texture payload stays below 12 MiB");
        const SurfaceMesh ultra (1024, 256);
        expect (ultra.payloadBytes()
                    + 1024 * 256 * sizeof (std::uint16_t)
                < 12 * 1024 * 1024);
    }
};

SurfaceMeshTests surfaceMeshTests;
} // namespace
