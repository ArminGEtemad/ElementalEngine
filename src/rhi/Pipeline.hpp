#pragma once

#include "RHICommon.hpp"
#include <cstdint>
#include <vector>

static constexpr uint32_t MAX_SPRINGS = 16;
namespace elementalEngine::RHI {
// Take this configuration structs to another scripts nect to the physics core
// TODO change the name later to match the fact that this is only for the stam
// fluid
struct SimConfig {
  uint32_t gridWidth;
  uint32_t gridHeight;
  float dt;
  float forceY;
  uint32_t numParticles;
  uint32_t pad[3];
};

// paticle based fluid for the acidic slime projectile
struct Particle {
  float position[2];
  float velocity[2];

  float predictedPosition[2];
  float density;
  float nearDensity;

  float pressure;
  float nearPressure;
  uint32_t pad[2];
};

// particle simulation parameters
// Clavet Paper
struct ParticleSimulationParameters {
  float dt;
  uint32_t numParticles;
  float interactionRadius;  // h
  float interactionRadius2; // h^2

  float restDensity;     // rho0
  float stiffness;       // k
  float nearStiffness;   // k^near
  float linearViscosity; // sigma

  float quadraticViscosity; // beta
  float springStiffness;    // k^spring
  float plasticity;         // alpha
  float yieldRatio;         // gamma

  float sticknessRadius;
  float sticknessMultiplier; // mu
  float cellSpacing;
  uint32_t hashGridSize;
};

struct Spring {
  uint32_t neighborID; // INVALID_ID if empty
  float restLength;
};

// -----------------------------------------------------------------------------
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