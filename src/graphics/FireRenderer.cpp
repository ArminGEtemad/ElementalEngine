#include "FireRenderer.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"

namespace elementalEngine::Renderer {
FireRenderer::FireRenderer(RHI::Device &device) : device(device) {
  createGraphicPipeline();
}

void FireRenderer::createGraphicPipeline() {
  using namespace RHI;
  PipelineConfig config;

  config.bindings = {
      {0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex}};
  config.pushConstants.size = sizeof(FireRenderParameters);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Vertex;

  config.blendMode = Blendmode::Additive;
  config.depthState.depthTestEnable = true;
  config.depthState.depthWriteEnable = false;
  config.depthState.depthCompareOp = CompareOp::Less;
  config.hasDepthAttachment = true;
  config.depthFormat = TextureFormat::D32_FLOAT;
  config.colorFormat = TextureFormat::B8G8R8A8_SRGB;
  config.cullMode = CullMode::None;

  firePipeline = device.createPipeline("fire_vs", "fire_fs", config);
}

void FireRenderer::draw(RHI::CommandList &commandList,
                        const Physics::FireSystem &fireSystem,
                        uint32_t screenWidth, uint32_t screenHeight,
                        const float *viewMatrix, const float *projMatrix) {

  const float colorDraw[4] = {1.0f, 0.65f, 0.15f, 1.0f};
  commandList.beginDebugMarker("Fire Particles Render", colorDraw);

  commandList.bindPipeline(*firePipeline);
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(screenWidth),
                          static_cast<float>(screenHeight));
  commandList.setScissor(0, 0, screenWidth, screenHeight);

  FireRenderParameters params{};
  std::memcpy(params.viewMatrix, viewMatrix, sizeof(float) * 16);
  std::memcpy(params.projMatrix, projMatrix, sizeof(float) * 16);
  params.domainWidth = 2000.0f;
  params.worldSizeX = 20.0f;

  commandList.pushConstants(0, sizeof(FireRenderParameters), &params,
                            RHI::ShaderStage::Vertex);

  // Bind the fire particle storage buffer from the physics system
  commandList.bindStorageBuffer(0, fireSystem.getParticleBuffer());

  // Draw 6 vertices (1 quad) per fire particle
  commandList.draw(6, fireSystem.getMaxParticles(), 0, 0);

  commandList.endDebugMarker();
}

} // namespace elementalEngine::Renderer