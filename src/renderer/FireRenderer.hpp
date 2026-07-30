#pragma once

#include "CommandList.hpp"
#include "Device.hpp"
#include "FireSystem.hpp"
#include "Pipeline.hpp"
#include <cstdint>
#include <memory>

namespace elementalEngine::Renderer {
struct FireRenderParameters {
  float viewProj[16];
};

class FireRenderer {

public:
  FireRenderer(RHI::Device &device);
  ~FireRenderer() = default;
  void draw(RHI::CommandList &commandList,
            const Physics::FireSystem &FireSystem, uint32_t screenWidth,
            uint32_t screenHeight, const float *viewProjMatrix);

private:
  RHI::Device &device;
  std::unique_ptr<RHI::Pipeline> firePipeline;

  void createGraphicPipeline();
};

} // namespace elementalEngine::Renderer