#pragma once

#include "RHICommon.hpp"
#include "Texture.hpp"
#include <cstdint>
#include <vector>

namespace elementalEngine::RHI {

enum class CullMode { None, Front, Back };

enum class CompareOp {
  Never,
  Less,
  Equal,
  LessOrEqual,
  Greater,
  GreaterOrEqual,
  NotEqual,
  Always
};

struct DepthState {
  bool depthTestEnable = true;
  bool depthWriteEnable = true;
  CompareOp depthCompareOp = CompareOp::Less;
};

enum class Blendmode { None, Additive, Alpha };

enum class ShaderStage : uint32_t {
  None = 0,
  Vertex = 1 << 0,
  Fragment = 1 << 1,
  Compute = 1 << 2,
  AllGraphics = Vertex | Fragment,
  All = Vertex | Fragment | Compute,
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
  return static_cast<ShaderStage>(static_cast<uint32_t>(a) |
                                  static_cast<uint32_t>(b));
}
inline bool operator&(ShaderStage a, ShaderStage b) {
  return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

enum class DescriptorType {
  Sampler,
  SampledImage,  // SRV
  StorageImage,  // UAV
  UniformBuffer, // CBV
  StorageBuffer
};

struct DescriptorBinding {
  uint32_t bindingSlot;
  DescriptorType type;
  uint32_t count = 1;
  ShaderStage stage;
};

struct PushConstantConfig {
  uint32_t size = 0;
  uint32_t offset = 0;
  ShaderStage stage = ShaderStage::None;
};

struct PipelineConfig {
  std::vector<DescriptorBinding> bindings;
  PushConstantConfig pushConstants;
  Blendmode blendMode = Blendmode::None; // default to None TODO Typo m -> M
  CullMode cullMode = CullMode::None;
  DepthState depthState{};
  TextureFormat colorFormat = TextureFormat::B8G8R8A8_SRGB;
  TextureFormat depthFormat = TextureFormat::D32_FLOAT;
  bool hasDepthAttachment = true;
};

class Pipeline {
public:
  virtual ~Pipeline() = default;

  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;

  // unifying compute and graphics pipeline
  virtual PipelineBindPoint getBindPoint() const = 0;

protected:
  Pipeline() = default;
};

} // namespace elementalEngine::RHI