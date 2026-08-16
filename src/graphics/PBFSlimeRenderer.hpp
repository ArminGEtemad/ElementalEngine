#include "CommandList.hpp"
#include "Device.hpp"
#include "physics/PBFSlime.hpp"

namespace elementalEngine::Renderer {
struct Slime3DPushConstants {
  float viewProj[16];
  float domainWidth;
  float domainHeight;
  float worldSizeX; // matches Terrain
  float worldSizeZ; // matches Terrain
  float particleRadius;
};

class PBFSlimeRenderer {
public:
  PBFSlimeRenderer(RHI::Device &device);
  ~PBFSlimeRenderer() = default;

  // pass 1
  void render3D(RHI::CommandList &commandList, uint32_t width, uint32_t height,
                const Physics::PBFSlime &slimeSimulation,
                const float *viewProjMatrix, float particleRadius,
                float worldSizeX = 20.0f, float worldSizeZ = 20.0f);

private:
  RHI::Device &device;
  // pipelines
  std::unique_ptr<RHI::Pipeline> slime3DPipeline;

  void createRenderingPipeline();
};
} // namespace elementalEngine::Renderer