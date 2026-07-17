#include "PBFSlimeRenderer.hpp"
#include "Pipeline.hpp"
#include "physics/PBFSlime.hpp"
#include <cstdint>

namespace elementalEngine::Renderer {
// Push Constants matching the Vertex Shader
struct PBFRenderParams {
  float viewProj[16];
  float particleRadius;
  float pad0[3];
};

PBFSlimeRenderer::PBFSlimeRenderer(RHI::Device &device) : device(device) {
  createRenderingPipeline();
}

void PBFSlimeRenderer::createRenderingPipeline() {
  using namespace RHI;

  // scope for pass 1 heightmap with additive blending
  {
    PipelineConfig config;
    // directly to the vertex for vertex pulling
    config.bindings = {
        {0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex}};

    config.pushConstants.size = sizeof(PBFRenderParams);
    config.pushConstants.offset = 0;
    config.pushConstants.stage = ShaderStage::Vertex;
    config.blendMode = Blendmode::Additive;

    heightmapPipeline = device.createPipeline("slime_heightmap_vs",
                                              "slime_heightmap_fs", config);
  }

  // scope for pass 2 composite and lighting without additive blending
  {
    PipelineConfig config;
    // directly to the vertex for vertex pulling
    config.bindings = {
        {0, DescriptorType::SampledImage, 1, ShaderStage::Fragment},
        {1, DescriptorType::Sampler, 1, ShaderStage::Fragment}};

    config.pushConstants.size = 0;
    config.blendMode = Blendmode::None;

    compositePipeline = device.createPipeline("slime_composite_vs",
                                              "slime_composite_fs", config);
  }
}

void PBFSlimeRenderer::ensureHeightmapTexture(uint32_t width, uint32_t height) {
  if (!heightmapTexture || heightmapTexture->getWidth() != width ||
      heightmapTexture->getHeight() != height) {
    heightmapTexture = device.createTexture(
        width, height, RHI::TextureFormat::B8G8R8A8_SRGB,
        RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource);
  }
}

void PBFSlimeRenderer::drawHeightmap(RHI::CommandList &commandList,
                                     const Physics::PBFSlime &slimeSimulation,
                                     const float *viewProjMatrix,
                                     float particleRadius) {

  ensureHeightmapTexture(2000, 800);
  // begin rendering offscreen texture
  commandList.beginRendering(heightmapTexture.get());

  commandList.bindPipeline(*heightmapPipeline);
  commandList.setViewport(0.0f, 0.0f, 2000.0f, 800.0f);
  commandList.setScissor(0, 0, 2000, 800);

  // Prepare and push the projection data
  PBFRenderParams params{};
  std::memcpy(params.viewProj, viewProjMatrix,
              sizeof(float) * 16); // 4x4 matrix
  params.particleRadius = particleRadius;
  commandList.pushConstants(0, sizeof(PBFRenderParams), &params,
                            RHI::ShaderStage::Vertex);

  // Bind the SSBO directly from the physics system
  commandList.bindStorageBuffer(0, slimeSimulation.getParticleBuffer());

  //  draw 6 vertices (1 quad) per particle.
  // No VBO is bound! The shader generates the geometry.
  commandList.draw(6, slimeSimulation.getParticleCount(), 0, 0);

  // end rendering to transition automatically the texture to shader resource
  commandList.endRendering(heightmapTexture.get());
}

void PBFSlimeRenderer::drawComposite(RHI::CommandList &commandList,
                                     uint32_t screenWidth,
                                     uint32_t screenHeight) {

  commandList.bindPipeline(*compositePipeline);
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(screenWidth),
                          static_cast<float>(screenHeight));
  commandList.setScissor(0, 0, screenWidth, screenHeight);

  // bind the accumulated heightmap texture and the linear sampler
  commandList.bindTexture(0, heightmapTexture.get());
  commandList.bindSampler(1);

  // draw 3 vertices to construct our full-screen triangle
  commandList.draw(3, 1, 0, 0);
}

} // namespace elementalEngine::Renderer