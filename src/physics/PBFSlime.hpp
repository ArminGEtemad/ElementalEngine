#pragma once

#include "Device.hpp"
#include "UsedParameters.hpp"
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
  ParticleSimulationParameters simParams;
  bool isFirstFrame = true;

  // resources
  std::unique_ptr<RHI::Buffer> particleBuffer;
  // Spatial Hashing Linked-List Buffers
  std::unique_ptr<RHI::Buffer> gridHeadBuffer; // 1 uint per cell
  std::unique_ptr<RHI::Buffer> gridNextBuffer; // 1 uint per particle
  std::unique_ptr<RHI::Buffer> springBuffer;

  // compute pipelines
  std::unique_ptr<RHI::Pipeline> clearGridPipeline;
  std::unique_ptr<RHI::Pipeline> buildGridPipeline;
  std::unique_ptr<RHI::Pipeline> predictionPipeline;
  std::unique_ptr<RHI::Pipeline> buildGridPredictedPipeline;
  std::unique_ptr<RHI::Pipeline> springPipeline;
  std::unique_ptr<RHI::Pipeline> densityPipeline;
  std::unique_ptr<RHI::Pipeline> relaxPipeline;
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