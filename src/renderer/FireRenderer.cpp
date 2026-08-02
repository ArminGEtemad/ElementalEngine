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
  firePipeline = device.createPipeline("fire_vs", "fire_fs", config);
}

void FireRenderer::draw(RHI::CommandList &commandList,
                        const Physics::FireSystem &fireSystem,
                        uint32_t screenWidth, uint32_t screenHeight,
                        const float *viewProjMatrix) {

  commandList.bindPipeline(*firePipeline);
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(screenWidth),
                          static_cast<float>(screenHeight));
  commandList.setScissor(0, 0, screenWidth, screenHeight);

  FireRenderParameters params{};
  std::memcpy(params.viewProj, viewProjMatrix, sizeof(float) * 16);
  commandList.pushConstants(0, sizeof(FireRenderParameters), &params,
                            RHI::ShaderStage::Vertex);

  // Bind the fire particle storage buffer from the physics system
  commandList.bindStorageBuffer(0, fireSystem.getParticleBuffer());

  // Draw 6 vertices (1 quad) per fire particle
  commandList.draw(6, fireSystem.getMaxParticles(), 0, 0);
}

} // namespace elementalEngine::Renderer