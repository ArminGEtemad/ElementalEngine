#pragma once

#include "RHICommon.hpp"
#include <cstdint>
#include <vector>

namespace elementalEngine::Graphics {

struct TerrainMesh {
  std::vector<RHI::Vertex3D> Vertices;
  std::vector<uint32_t> Indices;

  // flat ground grid centered around (0,0,0)
  static TerrainMesh generateGrid(uint32_t gridWidthCells,
                                  uint32_t gridDepthCells, float worldSizeX,
                                  float worldSizeZ) {
    TerrainMesh mesh;

    uint32_t numVertsX = gridWidthCells + 1;
    uint32_t numVertsZ = gridDepthCells + 1;

    mesh.Vertices.reserve(numVertsX * numVertsZ);

    float halfWidth = worldSizeX * 0.5f;
    float halfDepth = worldSizeZ * 0.5f;

    // generate Vertices across the X-Z floor plane
    for (uint32_t z = 0; z < numVertsZ; ++z) {
      for (uint32_t x = 0; x < numVertsX; ++x) {
        float u = static_cast<float>(x) / static_cast<float>(gridWidthCells);
        float v = static_cast<float>(z) / static_cast<float>(gridDepthCells);

        float posX = -halfWidth + u * worldSizeX;
        float posZ = -halfDepth + v * worldSizeZ;
        float posY = 0.0f;

        RHI::Vertex3D vert{
            {posX, posY, posZ, 1.0f}, // position
            {0.0f, 1.0f, 0.0f, 0.0f}, // normal
            {u, v, 0.0f, 0.0f}        // uv
        };

        mesh.Vertices.push_back(vert);
      }
    }

    // 2 triangles per quad
    mesh.Indices.reserve(gridWidthCells * gridDepthCells * 6);

    for (uint32_t z = 0; z < gridDepthCells; ++z) {
      for (uint32_t x = 0; x < gridWidthCells; ++x) {
        uint32_t topLeft = z * numVertsX + x;
        uint32_t topRight = topLeft + 1;
        uint32_t bottomLeft = (z + 1) * numVertsX + x;
        uint32_t bottomRight = bottomLeft + 1;

        // First triangle
        mesh.Indices.push_back(topLeft);
        mesh.Indices.push_back(bottomLeft);
        mesh.Indices.push_back(topRight);

        // Second triangle
        mesh.Indices.push_back(topRight);
        mesh.Indices.push_back(bottomLeft);
        mesh.Indices.push_back(bottomRight);
      }
    }

    return mesh;
  }
};

} // namespace elementalEngine::Graphics