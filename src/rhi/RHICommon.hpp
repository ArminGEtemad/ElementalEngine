#pragma once

#include <cstdint>
namespace elementalEngine::RHI {
enum class GraphicsAPI { Vulkan, DirectX12 };

enum class PipelineBindPoint { Graphics, Compute };

enum class ResourceState {
  Undefined,
  UnorderedAccess,
  ShaderResource,
  TransferDst
};

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

// test Vertex
struct Vertex {
  float position[2];
  float color[3];
};

} // namespace elementalEngine::RHI