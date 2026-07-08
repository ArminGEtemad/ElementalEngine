#pragma once

#include "Device.hpp"
#include <cstdint>
#include <memory>
namespace elementalEngine::Physics {
class PBFSlime {
public:
  PBFSlime(RHI::Device &device, uint32_t particleNumberMax);
  ~PBFSlime() = default;

  // called every frame
  void simulate(RHI::CommandList &cmdList, float dt);

  RHI::Buffer *getParticleBuffer() const { return particleBuffer.get(); }
  uint32_t getParticleCount() const { return numParticles; }

private:
  RHI::Device &device;
  uint32_t numParticles;
  RHI::ParticleSimulationParameters simParams;

  // resources
  std::unique_ptr<RHI::Buffer> particleBuffer;
  // Spatial Hashing Linked-List Buffers
  std::unique_ptr<RHI::Buffer> gridHeadBuffer; // 1 uint per cell
  std::unique_ptr<RHI::Buffer> gridNextBuffer; // 1 uint per particle

  // compute pipelines
  std::unique_ptr<RHI::Pipeline> predictPipeline;
  std::unique_ptr<RHI::Pipeline> hashPipeline;
  std::unique_ptr<RHI::Pipeline> solveLambdaPipeline;
  std::unique_ptr<RHI::Pipeline> solveDeltaPipeline;
  std::unique_ptr<RHI::Pipeline> integratePipeline;

  // create functions
  void createPipelines();
  void createRenderPipeline();
  void createResources();
  // for the slime it is ok to do on CPU but if I add massive amount of water
  // moving it to GPU
  void initializeParticles(); // cpu initialization (TODO move to GPU)
};
} // namespace elementalEngine::Physics