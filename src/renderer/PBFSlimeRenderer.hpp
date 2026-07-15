#include "CommandList.hpp"
#include "Device.hpp"
#include "physics/PBFSlime.hpp"

namespace elementalEngine::Renderer {
class PBFSlimeRenderer {
public:
  PBFSlimeRenderer(RHI::Device &device);
  ~PBFSlimeRenderer() = default;

  void draw(RHI::CommandList &commandList,
            const Physics::PBFSlime &slimeSimulation, uint32_t screenWidth,
            uint32_t screenHeight, const float *viewProjMatrix,
            float particleRadius);

private:
  RHI::Device &device;
  // pipelines
  std::unique_ptr<RHI::Pipeline> graphicsPipeline;
  void createRenderingPipeline();
};
} // namespace elementalEngine::Renderer