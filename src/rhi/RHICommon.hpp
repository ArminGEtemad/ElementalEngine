#pragma once

#include <cstdint>
namespace elementalEngine::RHI {
// common rhi definitions and promitives.
// backend agnostic abstraction layer.

// execution domain either graphics for vertex and fragment or compute for
// compute shaders
enum class PipelineBindPoint { Graphics, Compute };

// tracking the resourse state when transitioning the buffer or texture.
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

// validation layers can be toggled in main. keeping the GPU assusted one off as
// default. the actual toggle are changed in the main so if wanna switch toggles
// change it in main not here since these are overwritten
struct DeviceConfig {
  bool enableValidationLayers = true;
  bool enableGPUAssistedValidationLayer = false;
};

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

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
  return (static_cast<BufferUsage>(static_cast<uint32_t>(a) |
                                   static_cast<uint32_t>(b)));
}
inline bool operator&(BufferUsage a, BufferUsage b) {
  return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class MemoryProperty { GPULocal, CPUAccess };

// not to be changed in to float3. should stay 4 dimensional and if the 4th
// dimension is not use, it should be a padding for 16 byte alignment for gpu
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