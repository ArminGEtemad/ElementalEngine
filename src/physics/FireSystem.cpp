#include "FireSystem.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include "UsedParameters.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace elementalEngine::Physics {

FireSystem::FireSystem(RHI::Device &device, uint32_t maxParticles)
    : device(device), maxParticles(maxParticles) {
  std::random_device rd;
  randomEngine.seed(rd());

  simParams.buoyancy = 20.0f;
  simParams.drag = 0.8f;
  simParams.coolingRate = 0.70f;
  simParams.expansionRate = 8.0f;

  createResource();
  createPipeline();
}

void FireSystem::createResource() {
  size_t bufferSize = maxParticles * sizeof(FireParticles);

  particleBuffer = device.createBuffer(
      bufferSize, RHI::BufferUsage::Storage | RHI::BufferUsage::Vertex,
      RHI::MemoryProperty::GPULocal);

  std::vector<FireParticles> initialData(maxParticles);
  for (uint32_t i = 0; i < maxParticles; i++) {
    initialData[i].position[0] = emitterX;
    initialData[i].position[1] = emitterY;

    initialData[i].velocity[0] = initialSpeedX(randomEngine);
    initialData[i].velocity[1] = initialSpeedY(randomEngine);

    float lifeTime = lifeDist(randomEngine);
    initialData[i].life = (static_cast<float>(i) / maxParticles) * lifeTime;
    initialData[i].maxLife = lifeTime;
    initialData[i].temperature = 1.0f;
    initialData[i].particleRadius = 5.0f;
  }
}

void FireSystem::createPipeline() {
  using namespace RHI;
  PipelineConfig config;

  config.bindings = {
      {0, DescriptorType::StorageBuffer, 1, ShaderStage::Compute}};

  config.pushConstants.size = sizeof(FireSimParameters);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Compute;

  simulatePipeline = device.createComputePipeline("fire_compute", config);
}

void FireSystem::simulate(RHI::CommandList &commandList, float dt) {
  simParams.dt = dt;
  simParams.numParticles = maxParticles;
  simParams.emitterX = emitterX;
  simParams.emitterY = emitterY;
  simParams.isBurning = isBurning ? 1 : 0;
  commandList.bindPipeline(*simulatePipeline);
  commandList.bindStorageBuffer(0, particleBuffer.get());
  commandList.pushConstants(0, sizeof(simParams), &simParams,
                            RHI::ShaderStage::Compute);

  uint32_t groupX = (maxParticles + 255) / 256;
  commandList.dispatch(groupX, 1, 1);

  commandList.transitionBuffer(particleBuffer.get(),
                               RHI::ResourceState::UnorderedAccess,
                               RHI::ResourceState::UnorderedAccess);
}

} // namespace elementalEngine::Physics