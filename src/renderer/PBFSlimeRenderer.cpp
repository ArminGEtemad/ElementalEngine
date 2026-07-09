#include "PBFSlimeRenderer.hpp"
#include "Pipeline.hpp"
#include "physics/PBFSlime.hpp"

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
  PipelineConfig config;

  // directly to the vertex for vertex pulling
  config.bindings = {
      {0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex}};

  config.pushConstants.size = sizeof(PBFRenderParams);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Vertex;

  graphicsPipeline =
      device.createPipeline("pbf_particle_vs", "pbf_particle_fs", config);
}

void PBFSlimeRenderer::draw(RHI::CommandList &commandList,
                            const Physics::PBFSlime &slimeSimulation,
                            uint32_t screenWidth, uint32_t screenHeight,
                            const float *viewProjMatrix, float particleRadius) {

  commandList.bindPipeline(*graphicsPipeline);
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(screenWidth),
                          static_cast<float>(screenHeight));
  commandList.setScissor(0, 0, screenWidth, screenHeight);

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
}

} // namespace elementalEngine::Renderer