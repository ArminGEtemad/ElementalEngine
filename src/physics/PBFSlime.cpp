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

  simParams.dt =
      0.016f; // TODO dt must become unified accross all the simulations
  simParams.restDensity = 1.0f;
  simParams.stiffness = 1.0f;
  simParams.viscosity = 0.01f;
  simParams.particleMass = 1.0f;
  simParams.smoothingRadius = 2.5f;
  simParams.numParticles = numParticles;

  createResources();
  createPipelines();
  initializeParticles();
}

void PBFSlime::createResources() {
  using namespace RHI;
  size_t particleBufferSize = numParticles * sizeof(Particle);
  size_t numGridCells = 128 * 128; // TODO hardcoded for now change later

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
       ShaderStage::Compute} // gridNextBuffer
  };

  config.pushConstants.size = sizeof(ParticleSimulationParameters);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Compute;

  predictPipeline = device.createComputePipeline("pbf_predict", config);
  hashPipeline = device.createComputePipeline("pbf_hash", config);
  solveLambdaPipeline = device.createComputePipeline("pbf_lambda", config);
  solveDeltaPipeline = device.createComputePipeline("pbf_delta", config);
  integratePipeline = device.createComputePipeline("pbf_integrate", config);
}

void PBFSlime::initializeParticles() {
  // generate goo of particles
  std::vector<RHI::Particle> initialData(numParticles);
  std::default_random_engine generator;

  // Distribute particles in a visible starting area in mid-air
  std::uniform_real_distribution<float> distX(40.0f, 70.0f);
  std::uniform_real_distribution<float> distY(550.0f, 560.0f);

  for (uint32_t i = 0; i < numParticles; ++i) {
    initialData[i].position[0] = distX(generator);
    initialData[i].position[1] = distY(generator);

    // horizontal projectile velocity
    initialData[i].velocity[0] = 120.0f;
    initialData[i].velocity[1] = 0.0f;

    initialData[i].predictedPosition[0] = initialData[i].position[0];
    initialData[i].predictedPosition[1] = initialData[i].position[1];

    initialData[i].state = 0; // flying
    initialData[i].lambda =
        0.0f; // has to be calculated in pbf lambda and then sent to delta
  }

  // Upload to GPU
  void *mappedData = particleBuffer->map();
  std::memcpy(mappedData, initialData.data(),
              initialData.size() * sizeof(RHI::Particle));
  particleBuffer->unmap();
}

void PBFSlime::simulate(RHI::CommandList &cmdList, float dt) {
  using namespace RHI;
  simParams.dt = dt;

  uint32_t groupX = (numParticles + 255) / 256;

  // Helper to bind resources whenever all 4 are needed
  auto bindPBFResources = [&]() {
    cmdList.bindStorageBuffer(0, particleBuffer.get());
    cmdList.bindStorageBuffer(1, gridHeadBuffer.get());
    cmdList.bindStorageBuffer(2, gridNextBuffer.get());
    cmdList.pushConstants(0, sizeof(ParticleSimulationParameters), &simParams,
                          ShaderStage::Compute);
  };

  // ---------------------------------------------------------
  // PREDICT PASS
  // ---------------------------------------------------------
  cmdList.bindPipeline(*predictPipeline);
  cmdList.bindStorageBuffer(0, particleBuffer.get());
  cmdList.pushConstants(0, sizeof(ParticleSimulationParameters), &simParams,
                        ShaderStage::Compute);
  cmdList.dispatch(groupX, 1, 1);

  cmdList.transitionBuffer(particleBuffer.get(), ResourceState::UnorderedAccess,
                           ResourceState::UnorderedAccess);

  // ---------------------------------------------------------
  // FINDING NEIGHBORS PASS (SPATIAL HASHING)
  // ---------------------------------------------------------
  cmdList.bindPipeline(*hashPipeline);
  bindPBFResources(); // all the bindings are needed
  cmdList.dispatch(groupX, 1, 1);

  cmdList.transitionBuffer(gridHeadBuffer.get(), ResourceState::UnorderedAccess,
                           ResourceState::UnorderedAccess);
  cmdList.transitionBuffer(gridNextBuffer.get(), ResourceState::UnorderedAccess,
                           ResourceState::UnorderedAccess);

  // ---------------------------------------------------------
  // LAMBDA AND DELTA PASS
  // ---------------------------------------------------------
  const int SOLVER_ITERATIONS = 4;
  for (int i = 0; i < SOLVER_ITERATIONS; ++i) {
    // Lambda
    cmdList.bindPipeline(*solveLambdaPipeline);
    bindPBFResources();
    cmdList.dispatch(groupX, 1, 1);
    cmdList.transitionBuffer(particleBuffer.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::UnorderedAccess);

    // Delta
    cmdList.bindPipeline(*solveDeltaPipeline);
    bindPBFResources();
    cmdList.dispatch(groupX, 1, 1);
    cmdList.transitionBuffer(particleBuffer.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::UnorderedAccess);
  }

  // ---------------------------------------------------------
  // INTEGRATE AND ADD VISCOSITY PASS
  // ---------------------------------------------------------
  cmdList.bindPipeline(*integratePipeline);
  bindPBFResources();
  cmdList.dispatch(groupX, 1, 1);

  cmdList.transitionBuffer(particleBuffer.get(), ResourceState::UnorderedAccess,
                           ResourceState::ShaderResource);
}
} // namespace elementalEngine::Physics