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
  Present,
  IndirectArgument
};

enum class IndexType { Uint16, Uint32 };

struct DeviceConfig {
  bool enableValidationLayers = true;
  bool enableGPUAssistedValidatioLayer = false;
};

// buffer usage
enum class BufferUsage : uint32_t {
  None = 0,
  Vertex = 1 << 0,
  Index = 1 << 1,
  Uniform = 1 << 2,
  Storage = 1 << 3,
  TransferSrc = 1 << 4,
  TransferDst = 1 << 5,
  Indirect = 1 << 6,
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
  float position[4];
  float normal[4];
  float uv[4];
};

struct DrawIndirectCommand {
  uint32_t vertexCount;
  uint32_t instanceCount;
  uint32_t firstVertex;
  uint32_t firstInstance;
};

struct DrawIndexedIndirectCommand {
  uint32_t indexCount;
  uint32_t instanceCount;
  uint32_t firstIndex;
  uint32_t vertexOffest;
  uint32_t firstInstance;
};

} // namespace elementalEngine::RHI