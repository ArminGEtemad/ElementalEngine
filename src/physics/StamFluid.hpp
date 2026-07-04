#pragma once

#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"
#include <cstdint>
#include <memory>
namespace elementalEngine::Physics {
class StamFluid {
public:
  StamFluid(RHI::Device &device, uint32_t width, uint32_t height);
  ~StamFluid() = default;

  // initialize the undifined resource states
  void init(RHI::CommandList &cmdList);
  // called every frame
  void simulate(RHI::CommandList &cmdList, float dt);

  // getter
  RHI::Texture *getRenderTexture() const;

private:
  RHI::Device &device;
  uint32_t gridWidth;
  uint32_t gridHeight;
  RHI::SimConfig simConfig;

  bool useBufferPingToRead = true;

  // resources
  std::unique_ptr<RHI::Texture> densityPingTex;
  std::unique_ptr<RHI::Texture> densityPongTex;
  std::unique_ptr<RHI::Texture> velocityPingTex;
  std::unique_ptr<RHI::Texture> velocityPongTex;
  std::unique_ptr<RHI::Texture> divergenceTex;
  std::unique_ptr<RHI::Texture> pressurePingTex;
  std::unique_ptr<RHI::Texture> pressurePongTex;

  // pipelines
  std::unique_ptr<RHI::Pipeline> advectionPipeline;
  std::unique_ptr<RHI::Pipeline> divPipeline;
  std::unique_ptr<RHI::Pipeline> jacobiPipeline;
  std::unique_ptr<RHI::Pipeline> gradPipeline;

  // create functions
  void createPipeline();
  void createResources();
};

} // namespace elementalEngine::Physics