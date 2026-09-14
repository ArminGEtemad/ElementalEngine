#pragma once

#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include <cstdint>
#include <memory>
#include <random>

namespace elementalEngine::Physics {

struct FireParticles {
  float position[4]; // x, y, z, pad
  float velocity[4]; // x, y, z, pad
  float life;
  float maxLife;
  float temperature;
  float particleRadius;
};

struct FireSimParameters {
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
  float pad;
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

  void startFire() { isBurning = true; }
  void endFire() { isBurning = false; }

  void simulate(RHI::CommandList &commadList, float dt,
                RHI::Buffer *slimeBuffer, uint32_t slimeParticleCount);

  // getters
  RHI::Buffer *getParticleBuffer() const { return particleBuffer.get(); }
  uint32_t getMaxParticles() const { return maxParticles; }

private:
  RHI::Device &device;
  uint32_t maxParticles;
  bool isBurning{true};
  float emitterX{0.0f};
  float emitterY{0.0f};
  float emitterZ{0.0f};

  FireSimParameters simParams;

  std::unique_ptr<RHI::Buffer> particleBuffer;
  std::unique_ptr<RHI::Pipeline> simulatePipeline;

  std::default_random_engine randomEngine;
  std::uniform_real_distribution<float> baseSpreadXZ{-35.0f, 35.0f};
  std::uniform_real_distribution<float> initialSpeedY{20.0f, 150.0f};
  std::uniform_real_distribution<float> initialSpeedXZ{-20.0f, 20.0f};
  std::uniform_real_distribution<float> lifeDist{0.1f, 3.0f};

  void createPipeline();
  void createResource();
};

} // namespace elementalEngine::Physics