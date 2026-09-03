#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "StamFluid.hpp"
#include <glm/glm.hpp>

#include <memory>

namespace elementalEngine::Renderer {

struct GasRenderConfig {
  glm::mat4 invViewProj;
  glm::vec4 cameraPos;
  glm::vec4 domainMin;
  glm::vec4 domainMax;
};

class StamFluidRenderer {
public:
  StamFluidRenderer(RHI::Device &device);
  ~StamFluidRenderer() = default;

  void draw(RHI::CommandList &commandList,
            const elementalEngine::Physics::StamFluid &fluidSimulation,
            const glm::mat4 &invViewProj, const glm::vec3 &cameraPos,
            uint32_t screenWidth, uint32_t screenHeight,
            RHI::Texture *terrainDepthTexture);

private:
  RHI::Device &device;
  // pipelines
  std::unique_ptr<RHI::Pipeline> graphicsPipeline;
  void createRenderingPipeline();
};
} // namespace elementalEngine::Renderer