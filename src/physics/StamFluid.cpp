#include "StamFluid.hpp"
#include <cstdint>

namespace elementalEngine::Physics {
StamFluid::StamFluid(RHI::Device &device, uint32_t width, uint32_t height)
    : device(device), gridWidth(width), gridHeight(height) {
  simConfig.gridWidth = width;
  simConfig.gridHeight = height;
  simConfig.forceY = 9.8f;

  createResources();
  createPipeline();
}

void StamFluid::createResources() {

  using namespace RHI;
  TextureUsage rwTextureUsage = TextureUsage::ShaderResource |
                                TextureUsage::UnorderedAccess |
                                TextureUsage::TransferDst;

  // -- ping pong texture
  densityPingTex = device.createTexture(
      gridWidth, gridHeight, TextureFormat::R32_FLOAT, rwTextureUsage);
  densityPongTex = device.createTexture(
      gridWidth, gridHeight, TextureFormat::R32_FLOAT, rwTextureUsage);
  velocityPingTex = device.createTexture(
      gridWidth, gridHeight, TextureFormat::R32G32_FLOAT, rwTextureUsage);
  velocityPongTex = device.createTexture(
      gridWidth, gridHeight, TextureFormat::R32G32_FLOAT, rwTextureUsage);
  divergenceTex = device.createTexture(
      gridWidth, gridHeight, TextureFormat::R32_FLOAT, rwTextureUsage);
  pressurePingTex = device.createTexture(
      gridWidth, gridHeight, TextureFormat::R32_FLOAT, rwTextureUsage);
  pressurePongTex = device.createTexture(
      gridWidth, gridHeight, TextureFormat::R32_FLOAT, rwTextureUsage);
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

  // create pipelines
  advectionPipeline = device.createComputePipeline("advection", config);
  divPipeline = device.createComputePipeline("divergence", config);
  jacobiPipeline = device.createComputePipeline("jacobi", config);
  gradPipeline = device.createComputePipeline("gradient", config);
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

void StamFluid::simulate(RHI::CommandList &commandList, float dt) {
  using namespace RHI;
  simConfig.dt = dt;

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
  commandList.pushConstants(0, sizeof(SimConfig), &simConfig);
  commandList.bindTexture(1, densityRead);
  commandList.bindTexture(2, velocityStart);
  commandList.bindStorageImage(3, densityWrite);
  commandList.bindStorageImage(4, velocityAdvected);
  commandList.bindSampler(5);
  commandList.dispatch(gridWidth / 8, gridHeight / 8, 1);

  // DIVERGENCE PASS
  commandList.transitionTexture(velocityAdvected,
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
  commandList.transitionTexture(divergenceTex.get(),
                                ResourceState::ShaderResource,
                                ResourceState::UnorderedAccess);

  commandList.bindPipeline(*divPipeline);
  commandList.pushConstants(0, sizeof(SimConfig), &simConfig);
  commandList.bindTexture(1, velocityAdvected);
  commandList.bindStorageImage(3, divergenceTex.get());
  commandList.dispatch(gridWidth / 8, gridHeight / 8, 1);

  // JACOBI SOLVER PASS
  commandList.transitionTexture(divergenceTex.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
  commandList.bindPipeline(*jacobiPipeline);
  commandList.pushConstants(0, sizeof(SimConfig), &simConfig);
  commandList.bindTexture(2, divergenceTex.get());

  bool usePressurePing = true;
  const int JACOBI_ITERATIONS = 20;

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
    commandList.dispatch(gridWidth / 8, gridHeight / 8, 1);

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
  commandList.pushConstants(0, sizeof(SimConfig), &simConfig);
  commandList.bindTexture(1, finalPressure);
  commandList.bindTexture(2, velocityAdvected);
  commandList.bindStorageImage(3, velocityStart);
  commandList.dispatch(gridWidth / 8, gridHeight / 8, 1);

  // Transition densityWrite to Read so the Graphics pipeline can draw it!
  commandList.transitionTexture(densityWrite, ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
  commandList.transitionTexture(velocityStart, ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);

  useBufferPingToRead = !useBufferPingToRead;
}

RHI::Texture *StamFluid::getRenderTexture() const {
  // Since we toggled useBufferPingToRead at the end of simulate,
  // we return the buffer that we just WROTE to (which is now ping, if we were
  // using pong).
  return useBufferPingToRead ? densityPingTex.get() : densityPongTex.get();
}

} // namespace elementalEngine::Physics