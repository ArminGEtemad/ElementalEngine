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

// Particle scale parameters passed via push constants (camera matrices are in
// CameraFrameData UBO).
struct alignas(16) FireRenderParameters {
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
            const Physics::FireSystem &fireSystem, RHI::Buffer *cameraBuffer,
            uint32_t screenWidth, uint32_t screenHeight);

private:
  RHI::Device &device;
  std::unique_ptr<RHI::Pipeline> firePipeline;

  void createGraphicPipeline();
};

} // namespace elementalEngine::Renderer
