#pragma once

#include "CommandList.hpp"
#include "Device.hpp"
#include "FireSystem.hpp"
#include "Pipeline.hpp"
#include <cstdint>
#include <memory>

namespace elementalEngine::Renderer {
struct FireRenderParameters {
  float viewMatrix[16];
  float projMatrix[16];

  float domainWidth;
  float worldSizeX;
  float pad[2];
};

class FireRenderer {

public:
  FireRenderer(RHI::Device &device);
  ~FireRenderer() = default;

  void draw(RHI::CommandList &commandList,
            const Physics::FireSystem &fireSystem, uint32_t screenWidth,
            uint32_t screenHeight, const float *viewMatrix,
            const float *projMatrix);

private:
  RHI::Device &device;
  std::unique_ptr<RHI::Pipeline> firePipeline;

  void createGraphicPipeline();
};

} // namespace elementalEngine::Renderer