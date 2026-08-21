#include "PBFSlimeRenderer.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"
#include "physics/PBFSlime.hpp"

namespace elementalEngine::Renderer {

PBFSlimeRenderer::PBFSlimeRenderer(RHI::Device &device) : device(device) {
  createRenderingPipeline();
}

void PBFSlimeRenderer::createRenderingPipeline() {
  using namespace RHI;

  PipelineConfig config{};
  // binding 0: particle storage buffer for vertex pulling
  config.bindings = {
      {0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex}};

  config.pushConstants.size = sizeof(Slime3DPushConstants);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Vertex;

  // 3D depth and rasterizer
  config.cullMode = CullMode::None;
  config.depthState.depthTestEnable = true;
  config.depthState.depthWriteEnable = true;
  config.depthState.depthCompareOp = CompareOp::Less;
  config.colorFormat = TextureFormat::B8G8R8A8_SRGB;
  config.depthFormat = TextureFormat::D32_FLOAT;
  config.hasDepthAttachment = true;
  config.blendMode = Blendmode::None;

  slime3DPipeline = device.createPipeline("Slime_3d_vs", "Slime_3d_fs", config);
}

void PBFSlimeRenderer::render3D(RHI::CommandList &commandList, uint32_t width,
                                uint32_t height,
                                const Physics::PBFSlime &slimeSimulation,
                                const float *viewProjMatrix,
                                float particleRadius, float worldSizeX,
                                float worldSizeZ) {

  commandList.setViewport(0.0f, 0.0f, static_cast<float>(width),
                          static_cast<float>(height));
  commandList.setScissor(0, 0, width, height);
  // bind
  commandList.bindPipeline(*slime3DPipeline);

  // fill push constant
  Slime3DPushConstants pushConstants{};
  std::memcpy(pushConstants.viewProj, viewProjMatrix, sizeof(float) * 16);
  pushConstants.domainWidth = 2000.0f;   // Matches simParams.domainWidth
  pushConstants.domainHeight = 800.0f;   // Matches simParams.domainHeight
  pushConstants.worldSizeX = worldSizeX; // 20.0f
  pushConstants.worldSizeZ = worldSizeZ; // 20.0f
  pushConstants.particleRadius = particleRadius;

  commandList.pushConstants(0, sizeof(Slime3DPushConstants), &pushConstants,
                            RHI::ShaderStage::Vertex);

  // Bind Particle Storage Buffer (SSBO)
  commandList.bindStorageBuffer(0, slimeSimulation.getParticleBuffer());

  // Draw 6 vertices (1 quad billboard) per particle
  commandList.draw(6, slimeSimulation.getParticleCount(), 0, 0);
}

} // namespace elementalEngine::Renderer