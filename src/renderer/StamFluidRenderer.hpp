#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "StamFluid.hpp"
#include <memory>

namespace elementalEngine::Renderer {
class StamFluidRenderer {
public:
  StamFluidRenderer(RHI::Device &device);
  ~StamFluidRenderer() = default;

  void draw(RHI::CommandList &commandList,
            const elementalEngine::Physics::StamFluid &fluidSimulation,
            uint32_t screenWidth, uint32_t screenHeight);

private:
  RHI::Device &device;
  // pipelines
  std::unique_ptr<RHI::Pipeline> graphicsPipeline;
  void createRenderingPipeline();
};
} // namespace elementalEngine::Renderer