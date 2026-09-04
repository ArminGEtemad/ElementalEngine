#include "StamFluid.hpp"
#include "Pipeline.hpp"

namespace elementalEngine::Physics {
StamFluid::StamFluid(RHI::Device &device, uint32_t width, uint32_t height,
                     uint32_t depth)
    : device(device), gridWidth(width), gridHeight(height), gridDepth(depth) {
  simConfig.gridWidth = width;
  simConfig.gridHeight = height;
  simConfig.gridDepth = depth;
  simConfig.forceY = 9.8f;

  // hardcoded for now matches the slime and I could put everything together
  // actually
  simConfig.domainWidth = 2000.0f;
  simConfig.domainHeight = 2000.0f;
  simConfig.domainDepth = 2000.0f;

  createResources();
  createPipeline();
}

void StamFluid::createResources() {

  using namespace RHI;
  TextureUsage rwTextureUsage = TextureUsage::ShaderResource |
                                TextureUsage::UnorderedAccess |
                                TextureUsage::TransferDst;

  // -- ping pong texture
  densityPingTex =
      device.createTexture(gridWidth, gridHeight, TextureFormat::R32_FLOAT,
                           rwTextureUsage, gridDepth);
  densityPongTex =
      device.createTexture(gridWidth, gridHeight, TextureFormat::R32_FLOAT,
                           rwTextureUsage, gridDepth);

  velocityPingTex = device.createTexture(gridWidth, gridHeight,
                                         TextureFormat::R32G32B32A32_FLOAT,
                                         rwTextureUsage, gridDepth);
  velocityPongTex = device.createTexture(gridWidth, gridHeight,
                                         TextureFormat::R32G32B32A32_FLOAT,
                                         rwTextureUsage, gridDepth);

  divergenceTex =
      device.createTexture(gridWidth, gridHeight, TextureFormat::R32_FLOAT,
                           rwTextureUsage, gridDepth);
  pressurePingTex =
      device.createTexture(gridWidth, gridHeight, TextureFormat::R32_FLOAT,
                           rwTextureUsage, gridDepth);
  pressurePongTex =
      device.createTexture(gridWidth, gridHeight, TextureFormat::R32_FLOAT,
                           rwTextureUsage, gridDepth);

  // buffer to hold the injected density and verlocity
  size_t totalCells = gridWidth * gridHeight * gridDepth;
  injectionBuffer = device.createBuffer(totalCells * 4 * sizeof(uint32_t),
                                        RHI::BufferUsage::Storage |
                                            RHI::BufferUsage::TransferDst,
                                        RHI::MemoryProperty::GPULocal);
}

void StamFluid::createPipeline() {
  using namespace RHI;
  PipelineConfig config;

  // make the bindings
  config.bindings = {
      {1, DescriptorType::SampledImage, 1,
       ShaderStage::Compute}, // Read Density / Velocity
      {2, DescriptorType::SampledImage, 1,
       ShaderStage::Compute}, // Read Velocity / Divergence
      {3, DescriptorType::StorageImage, 1,
       ShaderStage::Compute}, // Write Density / Pressure
      {4, DescriptorType::StorageImage, 1,
       ShaderStage::Compute},                               // Write Velocity
      {5, DescriptorType::Sampler, 1, ShaderStage::Compute} // Linear Sampler
  };

  // push const buffer
  config.pushConstants.size = sizeof(SimConfig);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Compute;

  config.bindings.push_back(
      {6, DescriptorType::StorageBuffer, 1, ShaderStage::Compute});

  // create pipelines
  advectionPipeline = device.createComputePipeline("stam_advection", config);
  divPipeline = device.createComputePipeline("stam_divergence", config);
  jacobiPipeline = device.createComputePipeline("stam_jacobi", config);
  gradPipeline = device.createComputePipeline("stam_gradient", config);

  PipelineConfig injectConfig;
  injectConfig.bindings = {
      {0, RHI::DescriptorType::StorageBuffer, 1,
       RHI::ShaderStage::Compute}, // Particles (t0)
      {1, RHI::DescriptorType::StorageBuffer, 1,
       RHI::ShaderStage::Compute} // Injection Grid (u1)
  };
  injectConfig.pushConstants.size = sizeof(SimConfig);
  injectConfig.pushConstants.offset = 0;
  injectConfig.pushConstants.stage = RHI::ShaderStage::Compute;

  injectPipeline = device.createComputePipeline("stam_inject", injectConfig);
}

void StamFluid::init(RHI::CommandList &setupCmd) {
  using namespace RHI;
  // Transition all textures from Undefined to UnorderedAccess
  setupCmd.transitionTexture(densityPingTex.get(), ResourceState::Undefined,
                             ResourceState::UnorderedAccess);
  setupCmd.transitionTexture(densityPongTex.get(), ResourceState::Undefined,
                             ResourceState::UnorderedAccess);
  setupCmd.transitionTexture(velocityPingTex.get(), ResourceState::Undefined,
                             ResourceState::UnorderedAccess);
  setupCmd.transitionTexture(velocityPongTex.get(), ResourceState::Undefined,
                             ResourceState::UnorderedAccess);
  setupCmd.transitionTexture(pressurePingTex.get(), ResourceState::Undefined,
                             ResourceState::UnorderedAccess);
  setupCmd.transitionTexture(pressurePongTex.get(), ResourceState::Undefined,
                             ResourceState::UnorderedAccess);
  setupCmd.transitionTexture(divergenceTex.get(), ResourceState::Undefined,
                             ResourceState::UnorderedAccess);

  // Transition all back to ShaderResource
  setupCmd.transitionTexture(densityPingTex.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::ShaderResource);
  setupCmd.transitionTexture(densityPongTex.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::ShaderResource);
  setupCmd.transitionTexture(velocityPingTex.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::ShaderResource);
  setupCmd.transitionTexture(velocityPongTex.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::ShaderResource);
  setupCmd.transitionTexture(pressurePingTex.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::ShaderResource);
  setupCmd.transitionTexture(pressurePongTex.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::ShaderResource);
  setupCmd.transitionTexture(divergenceTex.get(),
                             ResourceState::UnorderedAccess,
                             ResourceState::ShaderResource);
}

void StamFluid::simulate(RHI::CommandList &commandList, float dt,
                         RHI::Buffer *particleBuffer, uint32_t numParticles) {
  using namespace RHI;
  simConfig.dt = dt;
  simConfig.numParticles = numParticles;

  uint32_t groupX = gridWidth / 8;
  uint32_t groupY = gridHeight / 8;
  uint32_t groupZ = gridDepth / 8;

  commandList.transitionBuffer(injectionBuffer.get(),
                               RHI::ResourceState::ShaderResource,
                               RHI::ResourceState::TransferDst);
  commandList.clearBuffer(injectionBuffer.get(), 0);
  commandList.transitionBuffer(injectionBuffer.get(),
                               RHI::ResourceState::TransferDst,
                               RHI::ResourceState::UnorderedAccess);

  if (particleBuffer && numParticles > 0) {
    // Run the injection scatter
    commandList.bindPipeline(*injectPipeline);
    commandList.pushConstants(0, sizeof(SimConfig), &simConfig,
                              RHI::ShaderStage::Compute);
    commandList.bindStorageBuffer(0, particleBuffer);
    commandList.bindStorageBuffer(1, injectionBuffer.get());

    uint32_t injectGroupX = (numParticles + 255) / 256;
    commandList.dispatch(injectGroupX, 1, 1);
  }

  // Transition buffer for the Advection shader to read it
  commandList.transitionBuffer(injectionBuffer.get(),
                               RHI::ResourceState::UnorderedAccess,
                               RHI::ResourceState::ShaderResource);

  Texture *densityRead =
      useBufferPingToRead ? densityPingTex.get() : densityPongTex.get();
  Texture *densityWrite =
      useBufferPingToRead ? densityPongTex.get() : densityPingTex.get();
  Texture *velocityStart = velocityPingTex.get();
  Texture *velocityAdvected = velocityPongTex.get();

  // ADVECTION PASS
  commandList.transitionTexture(densityWrite, ResourceState::ShaderResource,
                                ResourceState::UnorderedAccess);
  commandList.transitionTexture(velocityAdvected, ResourceState::ShaderResource,
                                ResourceState::UnorderedAccess);

  commandList.bindPipeline(*advectionPipeline);
  commandList.pushConstants(0, sizeof(SimConfig), &simConfig,
                            ShaderStage::Compute);
  commandList.bindTexture(1, densityRead);
  commandList.bindTexture(2, velocityStart);
  commandList.bindStorageImage(3, densityWrite);
  commandList.bindStorageImage(4, velocityAdvected);
  commandList.bindSampler(5);
  commandList.bindStorageBuffer(6, injectionBuffer.get());
  commandList.dispatch(groupX, groupY, groupZ);

  // DIVERGENCE PASS
  commandList.transitionTexture(velocityAdvected,
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
  commandList.transitionTexture(divergenceTex.get(),
                                ResourceState::ShaderResource,
                                ResourceState::UnorderedAccess);

  commandList.bindPipeline(*divPipeline);
  commandList.pushConstants(0, sizeof(SimConfig), &simConfig,
                            ShaderStage::Compute);
  commandList.bindTexture(1, velocityAdvected);
  commandList.bindStorageImage(3, divergenceTex.get());
  commandList.dispatch(groupX, groupY, groupZ);

  // JACOBI SOLVER PASS
  commandList.transitionTexture(divergenceTex.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
  commandList.bindPipeline(*jacobiPipeline);
  commandList.pushConstants(0, sizeof(SimConfig), &simConfig,
                            ShaderStage::Compute);
  commandList.bindTexture(2, divergenceTex.get());

  bool usePressurePing = true;
  const int JACOBI_ITERATIONS = 8;

  for (int i = 0; i < JACOBI_ITERATIONS; ++i) {
    Texture *pRead =
        usePressurePing ? pressurePingTex.get() : pressurePongTex.get();
    Texture *pWrite =
        usePressurePing ? pressurePongTex.get() : pressurePingTex.get();

    if (i > 0) {
      commandList.transitionTexture(pRead, ResourceState::UnorderedAccess,
                                    ResourceState::ShaderResource);
    }
    commandList.transitionTexture(pWrite, ResourceState::ShaderResource,
                                  ResourceState::UnorderedAccess);

    commandList.bindTexture(1, pRead);
    commandList.bindStorageImage(3, pWrite);
    commandList.dispatch(groupX, groupY, groupZ);

    usePressurePing = !usePressurePing;
  }

  // GRADIENT SUBTRACTION PASS
  Texture *finalPressure =
      usePressurePing ? pressurePingTex.get() : pressurePongTex.get();

  commandList.transitionTexture(finalPressure, ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
  commandList.transitionTexture(velocityStart, ResourceState::ShaderResource,
                                ResourceState::UnorderedAccess);

  commandList.bindPipeline(*gradPipeline);
  commandList.pushConstants(0, sizeof(SimConfig), &simConfig,
                            ShaderStage::Compute);
  commandList.bindTexture(1, finalPressure);
  commandList.bindTexture(2, velocityAdvected);
  commandList.bindStorageImage(3, velocityStart);
  commandList.dispatch(groupX, groupY, groupZ);

  // Transition densityWrite to Read so the Graphics pipeline can draw it!
  commandList.transitionTexture(densityWrite, ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
  commandList.transitionTexture(velocityStart, ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);

  useBufferPingToRead = !useBufferPingToRead;
}

RHI::Texture *StamFluid::getRenderTexture() const {
  return useBufferPingToRead ? densityPingTex.get() : densityPongTex.get();
}

} // namespace elementalEngine::Physics