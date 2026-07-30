#pragma once

#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "UsedParameters.hpp"
#include <cstdint>
#include <memory>
#include <random>

namespace elementalEngine::Physics {
class FireSystem {
public:
  FireSystem(RHI::Device &device, uint32_t maxParticles);
  ~FireSystem() = default;

  void setEmitterPosition(float x, float y) {
    emitterX = x;
    emitterY = y;
  }

  void startFire() { isBurning = true; }
  void endFire() { isBurning = false; }

  void simulate(RHI::CommandList &commadList, float dt);

  // getters
  RHI::Buffer *getParticleBuffer() const { return particleBuffer.get(); }
  uint32_t getMaxParticles() const { return maxParticles; }

private:
  RHI::Device &device;
  uint32_t maxParticles;
  bool isBurning{true};
  float emitterX{1000.0f};
  float emitterY{0.0f};

  FireSimParameters simParams;

  std::unique_ptr<RHI::Buffer> particleBuffer;
  std::unique_ptr<RHI::Pipeline> simulatePipeline;

  std::default_random_engine randomEngine;
  std::uniform_real_distribution<float> baseSpreadX{-35.0f, 35.0f};
  std::uniform_real_distribution<float> initialSpeedY{20.0f, 150.0f};
  std::uniform_real_distribution<float> initialSpeedX{-20.0f, 20.0f};
  std::uniform_real_distribution<float> lifeDist{0.1f, 3.0f};

  void createPipeline();
  void createResource();
};

} // namespace elementalEngine::Physics