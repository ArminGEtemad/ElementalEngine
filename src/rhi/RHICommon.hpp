#pragma once

#include <cstdint>
namespace elementalEngine::RHI {

enum class PipelineBindPoint { Graphics, Compute };

enum class ResourceState {
  Undefined,
  UnorderedAccess,
  ShaderResource,
  RenderTarget,
  DepthStencilWrite,
  DepthStencilReadOnly,
  TransferDst,
  TransferSrc,
  Present
};

enum class IndexType { Uint16, Uint32 };

struct DeviceConfig {
  bool enableValidationLayers = true;
  bool enableGPUAssistedValidatioLayer = false;
};

// buffer usage
enum class BufferUsage : uint32_t {
  None = 0,
  Vertex = 1 << 0,      // triangles
  Index = 1 << 1,       // index array
  Uniform = 1 << 2,     // uniform buffer for constants
  Storage = 1 << 3,     // massive data
  TransferSrc = 1 << 4, // source
  TransferDst = 1 << 5  // destination
};

// combining buffer usages
inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
  return (static_cast<BufferUsage>(static_cast<uint32_t>(a) |
                                   static_cast<uint32_t>(b)));
}
inline bool operator&(BufferUsage a, BufferUsage b) {
  return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// placed memory
enum class MemoryProperty { GPULocal, CPUAccess };

// 3D vertecies
struct Vertex3D {
  float position[4]; // 16 bytes (x, y, z, 1.0)
  float normal[4];   // 16 bytes (x, y, z, 0.0)
  float uv[4];       // 16 bytes (u, v, 0.0, 0.0)
}; // easier for 16 byte alignment

// legacy 2D version
struct Vertex2D {
  float position[2];
  float color[3];
};

} // namespace elementalEngine::RHI