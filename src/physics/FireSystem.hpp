#pragma once

#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include <cstdint>
#include <memory>
#include <random>

namespace elementalEngine::Physics {
// particle based fire simulation pass. (compute shader)
// simulates particle kinematics.
// gpu driven compute

// per particle state wutg 48 bytes total, aligned to 16 bytes for GPU.
struct alignas(16) FireParticles {
  float position[4];
  float velocity[4];
  float life;
  float maxLife;
  float temperature;
  float particleRadius;
};

struct alignas(16) FireSimParameters {
  float dt;
  uint32_t numParticles;
  float buoyancy;
  float drag;
  float coolingRate;
  float expansionRate;
  float emitterX;
  float emitterY;
  float emitterZ;
  uint32_t isBurning;
  uint32_t slimeParticleCount;
  float time;
};

class FireSystem {
public:
  FireSystem(RHI::Device &device, uint32_t maxParticles);
  ~FireSystem() = default;

  void setEmitterPosition(float x, float y, float z) {
    emitterX = x;
    emitterY = y;
    emitterZ = z;
  }
  // handles necessary transitions
  void simulate(RHI::CommandList &commadList, float dt, float totalTime,
                RHI::Buffer *slimeBuffer, uint32_t slimeParticleCount);

  void startFire() { isBurning = true; }
  void endFire() { isBurning = false; }

  // getters
  RHI::Buffer *getParticleBuffer() const { return particleBuffer.get(); }
  uint32_t getMaxParticles() const { return maxParticles; }
  RHI::Buffer *getAliveIndexBuffer() const { return aliveIndexBuffer.get(); }
  RHI::Buffer *getIndirectDrawBuffer() const {
    return indirectDrawBuffer.get();
  }

private:
  RHI::Device &device;
  uint32_t maxParticles;
  FireSimParameters simParams;

  bool isBurning{true};
  float emitterX{0.0f};
  float emitterY{0.0f};
  float emitterZ{0.0f};
  bool isFirstFrame{true};

  std::unique_ptr<RHI::Buffer> particleBuffer;
  std::unique_ptr<RHI::Pipeline> simulatePipeline;
  std::unique_ptr<RHI::Buffer> aliveIndexBuffer;
  std::unique_ptr<RHI::Buffer> indirectDrawBuffer;

  // intial distribution to make the start of the fire more dramatic and feel
  // like an explosion
  std::default_random_engine randomEngine;
  std::uniform_real_distribution<float> initialSpeedY{20.0f, 150.0f};
  std::uniform_real_distribution<float> initialSpeedXZ{-20.0f, 20.0f};
  std::uniform_real_distribution<float> lifeDist{0.1f, 3.0f};

  void createPipeline();
  void createResource();
};

} // namespace elementalEngine::Physics