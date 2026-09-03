#include "StamFluidRenderer.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"

namespace elementalEngine::Renderer {

StamFluidRenderer::StamFluidRenderer(RHI::Device &device) : device(device) {
  createRenderingPipeline();
}

void StamFluidRenderer::createRenderingPipeline() {
  using namespace RHI;
  PipelineConfig config;

  // because the raymarcher samples the depth it self there is no need for it in
  // the configs
  config.colorFormat = TextureFormat::B8G8R8A8_SRGB;
  config.blendMode = Blendmode::Alpha;
  config.depthState.depthTestEnable = false;
  config.cullMode = CullMode::None;

  config.bindings = {
      {1, DescriptorType::SampledImage, 1,
       ShaderStage::Fragment}, // Density (t1)
      {2, DescriptorType::Sampler, 1,
       ShaderStage::Fragment}, // Linear Sampler (s2)
      {3, DescriptorType::SampledImage, 1,
       ShaderStage::Fragment}, // Depth Texture (t3)
  };

  config.pushConstants.size = sizeof(GasRenderConfig);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Fragment;

  graphicsPipeline = device.createPipeline("grid_vs", "grid_fs", config);
}

void StamFluidRenderer::draw(RHI::CommandList &commandList,
                             const Physics::StamFluid &fluidSimulation,
                             const glm::mat4 &invViewProj,
                             const glm::vec3 &cameraPos, uint32_t screenWidth,
                             uint32_t screenHeight,
                             RHI::Texture *terrainDepthTexture) {

  // TODO start define these constants in one single script at some point!
  float worldSizeX = 20.0f;
  float worldSizeY = 20.0f;
  float worldSizeZ = 20.0f;

  GasRenderConfig renderConfig{};
  renderConfig.invViewProj = invViewProj;
  renderConfig.cameraPos = glm::vec4(cameraPos, 1.0f);

  renderConfig.domainMin =
      glm::vec4(-worldSizeX * 0.5f, 0.0f, -worldSizeZ * 0.5f, 1.0f);
  renderConfig.domainMax =
      glm::vec4(worldSizeX * 0.5f, worldSizeY, worldSizeZ * 0.5f, 1.0f);

  commandList.bindPipeline(*graphicsPipeline);
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(screenWidth),
                          static_cast<float>(screenHeight));
  commandList.setScissor(0, 0, screenWidth, screenHeight);

  commandList.pushConstants(0, sizeof(GasRenderConfig), &renderConfig,
                            RHI::ShaderStage::Fragment);

  commandList.bindTexture(1, fluidSimulation.getRenderTexture());
  commandList.bindSampler(2);
  commandList.bindTexture(3, terrainDepthTexture);

  commandList.draw(3, 1, 0, 0); // Fullscreen triangle
}

} // namespace elementalEngine::Renderer