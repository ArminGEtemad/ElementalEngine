#pragma once

#include "RHICommon.hpp"
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace elementalEngine::Graphics {

struct alignas(16) TerrainChunkData {
  glm::vec4 boundsMin;
  glm::vec4 boundsMax;
  uint32_t firstIndex;
  uint32_t indexCount;
  float pad[2];
};

struct TerrainMesh {
  std::vector<RHI::Vertex3D> Vertices;
  std::vector<uint32_t> Indices;
  std::vector<TerrainChunkData> Chunks;

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

    uint32_t cellsPerChunk = 8;
    uint32_t numChunksX = (gridWidthCells + cellsPerChunk - 1) / cellsPerChunk;
    uint32_t numChunksZ = (gridDepthCells + cellsPerChunk - 1) / cellsPerChunk;

    // 2 triangles per quad
    mesh.Indices.reserve(gridWidthCells * gridDepthCells * 6);
    mesh.Chunks.reserve(numChunksX * numChunksZ);

    for (uint32_t cz = 0; cz < numChunksZ; ++cz) {
      for (uint32_t cx = 0; cx < numChunksX; ++cx) {
        TerrainChunkData chunk{};
        chunk.firstIndex = static_cast<uint32_t>(mesh.Indices.size());
        float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
        float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;

        uint32_t startX = cx * cellsPerChunk;
        uint32_t startZ = cz * cellsPerChunk;
        uint32_t endX = std::min(startX + cellsPerChunk, gridWidthCells);
        uint32_t endZ = std::min(startZ + cellsPerChunk, gridDepthCells);

        for (uint32_t z = startZ; z < endZ; ++z) {
          for (uint32_t x = startX; x < endX; ++x) {
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

            uint32_t quadIndices[4] = {topLeft, topRight, bottomLeft,
                                       bottomRight};
            for (int i = 0; i < 4; ++i) {
              float vx = mesh.Vertices[quadIndices[i]].position[0];
              float vy = mesh.Vertices[quadIndices[i]].position[1];
              float vz = mesh.Vertices[quadIndices[i]].position[2];
              if (vx < minX)
                minX = vx;
              if (vy < minY)
                minY = vy;
              if (vz < minZ)
                minZ = vz;
              if (vx > maxX)
                maxX = vx;
              if (vy > maxY)
                maxY = vy;
              if (vz > maxZ)
                maxZ = vz;
            }
          }
        }
        chunk.indexCount =
            static_cast<uint32_t>(mesh.Indices.size()) - chunk.firstIndex;

        chunk.boundsMin = glm::vec4(minX, minY - 0.5f, minZ, 1.0f);
        chunk.boundsMax = glm::vec4(maxX, maxY + 0.5f, maxZ, 1.0f);

        mesh.Chunks.push_back(chunk);
      }
    }

    return mesh;
  }
};

} // namespace elementalEngine::Graphics