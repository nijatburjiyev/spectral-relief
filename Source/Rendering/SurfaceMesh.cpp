#include "SurfaceMesh.h"

#include <algorithm>

SurfaceMesh::SurfaceMesh (int requestedWidth, int requestedDepth)
    : width (std::max (2, requestedWidth)), depth (std::max (2, requestedDepth))
{
    vertices.reserve (static_cast<std::size_t> (width) * static_cast<std::size_t> (depth));
    for (int row = 0; row < depth; ++row)
    {
        const auto v = static_cast<float> (row) / static_cast<float> (depth - 1);
        for (int column = 0; column < width; ++column)
        {
            const auto u = static_cast<float> (column) / static_cast<float> (width - 1);
            vertices.push_back ({ 2.0f * u - 1.0f, 2.0f * v - 1.0f, u, v });
        }
    }

    indices.reserve (static_cast<std::size_t> (width - 1)
                     * static_cast<std::size_t> (depth - 1) * 6u);
    for (int row = 0; row < depth - 1; ++row)
    {
        for (int column = 0; column < width - 1; ++column)
        {
            const auto topLeft = static_cast<std::uint32_t> (row * width + column);
            const auto topRight = static_cast<std::uint32_t> (topLeft + 1);
            const auto bottomLeft = topLeft + static_cast<std::uint32_t> (width);
            const auto bottomRight = static_cast<std::uint32_t> (bottomLeft + 1);

            indices.insert (indices.end(),
                            { topLeft, topRight, bottomRight,
                              topLeft, bottomRight, bottomLeft });
        }
    }
}
