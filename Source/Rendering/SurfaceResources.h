#pragma once

#include <cstddef>

struct SurfaceResources
{
    unsigned int vertexArray = 0;
    unsigned int vertexBuffer = 0;
    unsigned int indexBuffer = 0;
    int indexCount = 0;
    int meshWidth = 0;
    int meshDepth = 0;
    std::size_t payloadBytes = 0;
};
