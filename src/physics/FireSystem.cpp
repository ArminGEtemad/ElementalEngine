#include "FireSystem.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
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

  // change if the fire kinematics does not feel realistic
  simParams.buoyancy = 50.0f;
  simParams.drag = 0.8f;
  simParams.coolingRate = 0.70f;
  simParams.expansionRate = 10.0f;

  createResource();
  createPipeline();
}

void FireSystem::createResource() {
  size_t bufferSize = maxParticles * sizeof(FireParticles);

  // vertex usage so the vertex shader fetches with no need for assembler
  particleBuffer = device.createBuffer(
      bufferSize,
      RHI::BufferUsage::Storage | RHI::BufferUsage::Vertex |
          RHI::BufferUsage::TransferDst,
      RHI::MemoryProperty::GPULocal);

  // needed for gpu driven rendering
  aliveIndexBuffer = device.createBuffer(maxParticles * sizeof(uint32_t),
                                         RHI::BufferUsage::Storage,
                                         RHI::MemoryProperty::GPULocal);
  indirectDrawBuffer = device.createBuffer(sizeof(RHI::DrawIndirectCommand),
                                           RHI::BufferUsage::Storage |
                                               RHI::BufferUsage::Indirect |
                                               RHI::BufferUsage::TransferDst,
                                           RHI::MemoryProperty::GPULocal);

  std::vector<FireParticles> initialData(maxParticles);
  for (uint32_t i = 0; i < maxParticles; i++) {
    initialData[i].position[0] = emitterX;
    initialData[i].position[1] = emitterY;
    initialData[i].position[2] = emitterZ;
    initialData[i].position[3] = 0.0f; // padding

    initialData[i].velocity[0] = initialSpeedXZ(randomEngine);
    initialData[i].velocity[1] = initialSpeedY(randomEngine);
    initialData[i].velocity[2] = initialSpeedXZ(randomEngine);
    initialData[i].velocity[3] = 0.0f; // padding

    float lifeTime = lifeDist(randomEngine);
    initialData[i].life = (static_cast<float>(i) / maxParticles) * lifeTime;
    initialData[i].maxLife = lifeTime;
    initialData[i].temperature = 1.0f;
    initialData[i].particleRadius = 2.0f;
  }

  device.uploadBufferData(particleBuffer.get(), initialData.data(), bufferSize);
}

void FireSystem::createPipeline() {
  using namespace RHI;
  PipelineConfig config;

  config.bindings = {
      {0, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute}, // 0 fire particles
      {1, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute}, // 1 slime particles
      {2, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute}, // 2 alive indices
      {3, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute} // 3 indirect command
  };
  config.pushConstants.size = sizeof(FireSimParameters);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Compute;

  simulatePipeline = device.createComputePipeline("fire_compute", config);
}

void FireSystem::simulate(RHI::CommandList &commandList, float dt,
                          float totalTime, RHI::Buffer *slimeBuffer,
                          uint32_t slimeParticleCount) {

  // debig color marker Amber / Orange
  const float colorSim[4] = {0.95f, 0.45f, 0.1f, 1.0f};
  commandList.beginDebugMarker("Fire Particle Simulation", colorSim);

  if (slimeBuffer) {
    commandList.transitionBuffer(slimeBuffer,
                                 RHI::ResourceState::ShaderResource);
  }
  commandList.transitionBuffer(particleBuffer.get(),
                               RHI::ResourceState::UnorderedAccess);
  commandList.transitionBuffer(indirectDrawBuffer.get(),
                               RHI::ResourceState::TransferDst);
  commandList.clearBuffer(indirectDrawBuffer.get(), 0);
  commandList.transitionBuffer(indirectDrawBuffer.get(),
                               RHI::ResourceState::UnorderedAccess);
  commandList.transitionBuffer(aliveIndexBuffer.get(),
                               RHI::ResourceState::UnorderedAccess);

  simParams.dt = dt;
  simParams.numParticles = maxParticles;
  simParams.emitterX = emitterX;
  simParams.emitterY = emitterY;
  simParams.emitterZ = emitterZ;
  simParams.isBurning = isBurning ? 1 : 0;
  simParams.slimeParticleCount = slimeParticleCount;
  simParams.time = totalTime;

  commandList.bindPipeline(*simulatePipeline);

  commandList.bindStorageBuffer(0, particleBuffer.get());
  if (slimeBuffer) {
    commandList.bindStorageBuffer(1, slimeBuffer);
  }
  commandList.bindStorageBuffer(2, aliveIndexBuffer.get());
  commandList.bindStorageBuffer(3, indirectDrawBuffer.get());
  commandList.pushConstants(0, sizeof(simParams), &simParams,
                            RHI::ShaderStage::Compute);

  uint32_t groupX = (maxParticles + 255) / 256;
  commandList.dispatch(groupX, 1, 1);

  commandList.transitionBuffer(particleBuffer.get(),
                               RHI::ResourceState::ShaderResource);
  commandList.transitionBuffer(indirectDrawBuffer.get(),
                               RHI::ResourceState::IndirectArgument);
  commandList.transitionBuffer(aliveIndexBuffer.get(),
                               RHI::ResourceState::ShaderResource);

  commandList.endDebugMarker();
}

} // namespace elementalEngine::Physics
