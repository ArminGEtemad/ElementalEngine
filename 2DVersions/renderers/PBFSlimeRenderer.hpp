#include "CommandList.hpp"
#include "Device.hpp"
#include "physics/PBFSlime.hpp"

namespace elementalEngine::Renderer {
class PBFSlimeRenderer {
public:
  PBFSlimeRenderer(RHI::Device &device);
  ~PBFSlimeRenderer() = default;

  // pass 1
  void drawHeightmap(RHI::CommandList &commandList,
                     const Physics::PBFSlime &slimeSimulation,
                     const float *viewProjMatrix, float particleRadius);
  // pass 2
  void drawComposite(RHI::CommandList &commandList, uint32_t screenWidth,
                     uint32_t screenHeight);

private:
  RHI::Device &device;
  // pipelines
  std::unique_ptr<RHI::Pipeline> heightmapPipeline;
  std::unique_ptr<RHI::Pipeline> compositePipeline;

  std::unique_ptr<RHI::Texture> heightmapTexture;

  void createRenderingPipeline();
  void ensureHeightmapTexture(uint32_t width, uint32_t height);
};
} // namespace elementalEngine::Renderer