#include "PBFSlime.hpp"
#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace elementalEngine::Physics {
PBFSlime::PBFSlime(RHI::Device &device, uint32_t particleNumberMax)
    : device(device), numParticles(particleNumberMax) {

  simParams.dt = 0.008f;
  simParams.numParticles = particleNumberMax;
  simParams.interactionRadius = 40.0f;
  simParams.interactionRadius2 =
      simParams.interactionRadius * simParams.interactionRadius;
  simParams.restDensity = 8.0f;
  simParams.stiffness = 15.0f;
  simParams.nearStiffness = 2.0f;
  simParams.linearViscosity = 0.5f;
  simParams.quadraticViscosity = 0.05f;
  simParams.springStiffness = 20.0f;
  simParams.plasticity = 10.0f;
  simParams.yieldRatio = 0.1f;
  simParams.sticknessRadius = 2.0f;

  simParams.sticknessRadius = 2.0f;
  simParams.sticknessMultiplier = 0.2f;
  simParams.cellSpacing = simParams.interactionRadius;
  simParams.hashGridSize = 700 * 300;

  createResources();
  createPipelines();
  initializeParticles();
}

void PBFSlime::createResources() {
  using namespace RHI;
  size_t particleBufferSize = numParticles * sizeof(Particle);
  size_t numGridCells = 700 * 300; // TODO hardcoded for now change later
  size_t springBufferSize = numParticles * MAX_SPRINGS * sizeof(Spring);

  // initialized in cpu
  particleBuffer = device.createBuffer(
      particleBufferSize, BufferUsage::Storage | BufferUsage::Vertex,
      MemoryProperty::CPUAccess);
  gridHeadBuffer =
      device.createBuffer(numGridCells * sizeof(uint32_t),
                          BufferUsage::Storage | BufferUsage::TransferDst,
                          MemoryProperty::GPULocal);
  gridNextBuffer =
      device.createBuffer(numParticles * sizeof(uint32_t), BufferUsage::Storage,
                          MemoryProperty::GPULocal);
  springBuffer = device.createBuffer(
      springBufferSize, BufferUsage::Storage | BufferUsage::TransferDst,
      MemoryProperty::GPULocal);
}

void PBFSlime::createPipelines() {
  using namespace RHI;
  PipelineConfig config;

  // PBF stages
  config.bindings = {
      {0, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute}, // particleBuffer
      {1, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute}, // gridHeadBuffer
      {2, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute}, // gridNextBuffer
      {3, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute} // springBuffer
  };

  config.pushConstants.size = sizeof(ParticleSimulationParameters);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Compute;

  buildGridPipeline =
      device.createComputePipeline("clavet_grid_construct", config);
  predictionPipeline = device.createComputePipeline("clavet_predicted", config);
  buildGridPredictedPipeline =
      device.createComputePipeline("clavet_grid_next_construct", config);
  springPipeline = device.createComputePipeline("clavet_springs", config);
  densityPipeline = device.createComputePipeline("clavet_density", config);
  relaxPipeline = device.createComputePipeline("clavet_relax", config);
  integratePipeline = device.createComputePipeline("clavet_integrate", config);
}

void PBFSlime::initializeParticles() {
  std::vector<Particle> initialData(numParticles);
  std::default_random_engine generator;

  // Widen the spawn cloud
  std::uniform_real_distribution<float> noiseXZ(-150.0f, 150.0f);
  std::uniform_real_distribution<float> spawnY(200.0f, 450.0f);

  for (uint32_t i = 0; i < numParticles; ++i) {
    initialData[i].position[0] = noiseXZ(generator); // X
    initialData[i].position[1] = spawnY(generator);  // Y (Height)
    initialData[i].position[2] = noiseXZ(generator); // Z (Depth)
    initialData[i].position[3] = 0.0f;               // pad

    initialData[i].velocity[0] = 800.0f; // Move slightly right
    initialData[i].velocity[1] = 200.0f; // Shoot UP
    initialData[i].velocity[2] = 0.0f;   // Shoot FORWARD
    initialData[i].velocity[3] = 0.0f;   // pad

    initialData[i].predictedPosition[0] = initialData[i].position[0];
    initialData[i].predictedPosition[1] = initialData[i].position[1];
    initialData[i].predictedPosition[2] = initialData[i].position[2];
    initialData[i].predictedPosition[3] = 0.0f;
  }

  void *mappedData = particleBuffer->map();
  std::memcpy(mappedData, initialData.data(),
              initialData.size() * sizeof(Particle));
  particleBuffer->unmap();
}

void PBFSlime::simulate(RHI::CommandList &cmdList, float dt, float strikeX,
                        float strikeY, float lightningOpacity) {
  using namespace RHI;
  simParams.dt = dt;

  if (!isFirstFrame) {
    cmdList.transitionBuffer(particleBuffer.get(),
                             RHI::ResourceState::ShaderResource,
                             RHI::ResourceState::UnorderedAccess);
  }

  uint32_t groupX = (numParticles + 255) / 256;
  if (isFirstFrame) {
    cmdList.transitionBuffer(springBuffer.get(), ResourceState::UnorderedAccess,
                             ResourceState::TransferDst);

    cmdList.clearBuffer(springBuffer.get(), 0xFFFFFFFF);

    cmdList.transitionBuffer(springBuffer.get(), ResourceState::TransferDst,
                             ResourceState::UnorderedAccess);
    isFirstFrame = false;
  }

  // Helper to bind resources whenever all 4 are needed
  auto bindPBFResources = [&]() {
    cmdList.bindStorageBuffer(0, particleBuffer.get());
    cmdList.bindStorageBuffer(1, gridHeadBuffer.get());
    cmdList.bindStorageBuffer(2, gridNextBuffer.get());
    cmdList.bindStorageBuffer(3, springBuffer.get());
    cmdList.pushConstants(0, sizeof(ParticleSimulationParameters), &simParams,
                          ShaderStage::Compute);
  };

  auto computeBarrier = [&]() {
    cmdList.transitionBuffer(particleBuffer.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::UnorderedAccess);
    cmdList.transitionBuffer(gridHeadBuffer.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::UnorderedAccess);
    cmdList.transitionBuffer(gridNextBuffer.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::UnorderedAccess);
    cmdList.transitionBuffer(springBuffer.get(), ResourceState::UnorderedAccess,
                             ResourceState::UnorderedAccess);
  };

  // clear buffer
  cmdList.transitionBuffer(gridHeadBuffer.get(), ResourceState::UnorderedAccess,
                           ResourceState::TransferDst);
  cmdList.clearBuffer(gridHeadBuffer.get(), 0xFFFFFFFF);
  cmdList.transitionBuffer(gridHeadBuffer.get(), ResourceState::TransferDst,
                           ResourceState::UnorderedAccess);

  // build grid
  cmdList.bindPipeline(*buildGridPipeline);
  bindPBFResources();
  cmdList.dispatch(groupX, 1, 1);
  computeBarrier();

  // build predicted
  cmdList.bindPipeline(*predictionPipeline);
  bindPBFResources();
  cmdList.dispatch(groupX, 1, 1);
  computeBarrier();

  // clear buffer
  cmdList.transitionBuffer(gridHeadBuffer.get(), ResourceState::UnorderedAccess,
                           ResourceState::TransferDst);
  cmdList.clearBuffer(gridHeadBuffer.get(), 0xFFFFFFFF);
  cmdList.transitionBuffer(gridHeadBuffer.get(), ResourceState::TransferDst,
                           ResourceState::UnorderedAccess);

  // build grid predicted and viscosity
  cmdList.bindPipeline(*buildGridPredictedPipeline);
  cmdList.bindStorageBuffer(0, particleBuffer.get());
  bindPBFResources();
  cmdList.dispatch(groupX, 1, 1);
  computeBarrier();

  // spring
  cmdList.bindPipeline(*springPipeline);
  cmdList.bindStorageBuffer(0, particleBuffer.get());
  bindPBFResources();
  cmdList.dispatch(groupX, 1, 1);
  computeBarrier();

  // density phase 1
  cmdList.bindPipeline(*densityPipeline);
  cmdList.bindStorageBuffer(0, particleBuffer.get());
  bindPBFResources();
  cmdList.dispatch(groupX, 1, 1);
  computeBarrier();

  // density phase 2
  cmdList.bindPipeline(*relaxPipeline);
  cmdList.bindStorageBuffer(0, particleBuffer.get());
  bindPBFResources();
  cmdList.dispatch(groupX, 1, 1);
  computeBarrier();

  // integrate
  cmdList.bindPipeline(*integratePipeline);
  cmdList.bindStorageBuffer(0, particleBuffer.get());
  cmdList.pushConstants(0, sizeof(ParticleSimulationParameters), &simParams,
                        ShaderStage::Compute);
  cmdList.dispatch(groupX, 1, 1);

  // Transition the buffer so your Graphics Pipeline can read the positions to
  // render them!
  cmdList.transitionBuffer(particleBuffer.get(), ResourceState::UnorderedAccess,
                           ResourceState::ShaderResource);
}
} // namespace elementalEngine::Physics