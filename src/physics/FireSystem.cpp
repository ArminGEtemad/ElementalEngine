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

  simParams.buoyancy = 10.0f;
  simParams.drag = 0.8f;
  simParams.coolingRate = 0.70f;
  simParams.expansionRate = 12.0f;

  createResource();
  createPipeline();
}

void FireSystem::respawnParticle(FireParticles &p) {
  // spawn at the flame base
  p.position[0] = emitterX + baseSpreadX(randomEngine);
  p.position[1] = emitterY;

  // initial main velocity updawards
  p.velocity[0] = initialSpeedX(randomEngine);
  p.velocity[1] = initialSpeedY(randomEngine);

  float lifeTime = lifeDist(randomEngine);
  p.life = lifeTime;
  p.maxLife = lifeTime;
  p.temperature = 1.0f;
  p.particleRadius = 3.0f;
}

void FireSystem::createResource() {
  size_t bufferSize = maxParticles * sizeof(FireParticles);

  particleBuffer = device.createBuffer(
      bufferSize, RHI::BufferUsage::Storage | RHI::BufferUsage::Vertex,
      RHI::MemoryProperty::CPUAccess);

  std::vector<FireParticles> initialData(maxParticles);
  for (auto &p : initialData) {
    respawnParticle(p);
    p.life = lifeDist(randomEngine); // so they don't die at the same time
  }

  void *mapped = particleBuffer->map();
  std::memcpy(mapped, initialData.data(), bufferSize);
  particleBuffer->unmap();
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
  void *mapped = particleBuffer->map();
  auto *particles = static_cast<FireParticles *>(mapped);

  for (uint32_t i = 0; i < maxParticles; ++i) {
    if (particles[i].life < 0.0f && isBurning) {
      respawnParticle(particles[i]);
    }
  }
  particleBuffer->unmap();

  simParams.dt = dt;
  simParams.numParticles = maxParticles;
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