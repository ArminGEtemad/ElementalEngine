#pragma once

#include "CommandList.hpp"
#include "Device.hpp"
#include "FireSystem.hpp"
#include "Pipeline.hpp"
#include <cstdint>
#include <memory>

namespace elementalEngine::Renderer {
// rendering fire particles and creating the graphics pipeline of the fire
// system. already moved to gpu driven rendering

// Camera transforms and particle scale parameters passed via push constants.
struct alignas(16) FireRenderParameters {
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

  // Records the indirect draw call for all alive fire particles into the
  // active render pass.
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