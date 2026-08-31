#include "StamFluidRenderer.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"
namespace elementalEngine::Renderer {
StamFluidRenderer::StamFluidRenderer(RHI::Device &device) : device(device) {
  createRenderingPipeline();
}

void StamFluidRenderer::createRenderingPipeline() {
  using namespace RHI;
  PipelineConfig GraphicPipelineConfig;

  GraphicPipelineConfig.colorFormat = TextureFormat::B8G8R8A8_SRGB;
  GraphicPipelineConfig.blendMode = Blendmode::Alpha;
  GraphicPipelineConfig.depthState.depthTestEnable = false;
  GraphicPipelineConfig.cullMode = CullMode::None;
  GraphicPipelineConfig.bindings = {
      {1, DescriptorType::SampledImage, 1, ShaderStage::Fragment},
      {3, DescriptorType::SampledImage, 1, ShaderStage::Fragment}};
  GraphicPipelineConfig.pushConstants.size = sizeof(Physics::SimConfig);
  GraphicPipelineConfig.pushConstants.stage = ShaderStage::Fragment;

  graphicsPipeline =
      device.createPipeline("grid_vs", "grid_fs", GraphicPipelineConfig);
}

void StamFluidRenderer::draw(RHI::CommandList &commandList,
                             const Physics::StamFluid &fluidSimulation,
                             uint32_t screenWidth, uint32_t screenHeight) {
  Physics::SimConfig configData = fluidSimulation.getSimConfig();

  commandList.bindPipeline(*graphicsPipeline);
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(screenWidth),
                          static_cast<float>(screenHeight));
  commandList.setScissor(0, 0, screenWidth, screenHeight);
  commandList.pushConstants(0, sizeof(Physics::SimConfig), &configData,
                            RHI::ShaderStage::Fragment);
  commandList.bindTexture(1, fluidSimulation.getRenderTexture());

  commandList.draw(3, 1, 0, 0); // Fullscreen Triangle
}

} // namespace elementalEngine::Renderer