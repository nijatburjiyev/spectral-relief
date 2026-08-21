#pragma once

#include <cstdint>
#include <vector>

struct SurfaceVertex
{
    float x;
    float z;
    float u;
    float v;
};

class SurfaceMesh
{
public:
    explicit SurfaceMesh (int width = 256, int depth = 256);

    [[nodiscard]] bool uses32BitIndices() const noexcept { return true; }
    [[nodiscard]] std::size_t payloadBytes() const noexcept
    {
        return vertices.size() * sizeof (SurfaceVertex)
             + indices.size() * sizeof (std::uint32_t);
    }

    int width;
    int depth;
    std::vector<SurfaceVertex> vertices;
    std::vector<std::uint32_t> indices;
};
