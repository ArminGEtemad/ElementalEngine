#pragma once

#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"
#include <cstdint>
#include <memory>

namespace elementalEngine::Physics {

struct SimConfig {
  uint32_t gridWidth;
  uint32_t gridHeight;
  uint32_t gridDepth;
  float dt;

  float forceY;
  uint32_t numParticles;
  float domainWidth;
  float domainHeight;

  float domainDepth;
  float pad[3];
};

class StamFluid {
public:
  StamFluid(RHI::Device &device, uint32_t width, uint32_t height,
            uint32_t depth);
  ~StamFluid() = default;

  void init(RHI::CommandList &commandList);
  void simulate(RHI::CommandList &commandList, float dt,
                RHI::Buffer *particleBuffer = nullptr,
                uint32_t numParticles = 0);

  // getters
  SimConfig getSimConfig() const { return simConfig; }
  RHI::Texture *getRenderTexture() const;

private:
  RHI::Device &device;

  uint32_t gridWidth;
  uint32_t gridHeight;
  uint32_t gridDepth;

  SimConfig simConfig;

  bool useBufferPingToRead = true;

  // resources
  std::unique_ptr<RHI::Texture> densityPingTex;
  std::unique_ptr<RHI::Texture> densityPongTex;
  std::unique_ptr<RHI::Texture> velocityPingTex;
  std::unique_ptr<RHI::Texture> velocityPongTex;
  std::unique_ptr<RHI::Texture> divergenceTex;
  std::unique_ptr<RHI::Texture> pressurePingTex;
  std::unique_ptr<RHI::Texture> pressurePongTex;
  std::unique_ptr<RHI::Buffer> injectionBuffer;

  // pipelines
  std::unique_ptr<RHI::Pipeline> injectPipeline;
  std::unique_ptr<RHI::Pipeline> advectionPipeline;
  std::unique_ptr<RHI::Pipeline> divPipeline;
  std::unique_ptr<RHI::Pipeline> jacobiPipeline;
  std::unique_ptr<RHI::Pipeline> gradPipeline;

  // create functions
  void createPipeline();
  void createResources();
};

} // namespace elementalEngine::Physics