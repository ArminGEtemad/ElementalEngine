#pragma once

#include "CommandList.hpp"
#include "Device.hpp"
#include "PBFSlime.hpp"
#include "Texture.hpp"
#include <cstdint>

namespace elementalEngine::Renderer {

struct SSFRPushConstants {
  float viewMatrix[16];
  float projMatrix[16];
  float domainWidth;
  float domainDepth;
  float worldSizeX;
  float worldSizeZ;
  float particleRadius;
  float pad[3];
};

class SSFRRenderer {
public:
  SSFRRenderer(RHI::Device &device, uint32_t width, uint32_t height);
  ~SSFRRenderer() = default;

  void onResize(uint32_t width, uint32_t height);

  void renderGBuffer(RHI::CommandList &commandList,
                     const Physics::PBFSlime &slime, const float *viewMatrix,
                     const float *projMatrix, float particleRadius,
                     float worldSizeX = 20.0f, float worldSizeZ = 20.0f);

  void renderBlur(RHI::CommandList &cmdList);

  void renderComposite(RHI::CommandList &cmdList,
                       RHI::Texture *swapchainTexture,
                       RHI::Texture *terrainDepthTexture,
                       const float *invViewMatrix, const float *invProjMatrix,
                       const float *projMatrix, const float *lightDir);

  RHI::Texture *getDepthTexture() const { return blurredDepthTexture.get(); }
  RHI::Texture *getThicknessTexture() const {
    return fluidThicknessTexture.get();
  }

private:
  void createRenderTargets(uint32_t width, uint32_t height);
  void createPipelines();

  RHI::Device &device;
  uint32_t currentWidth;
  uint32_t currentHeight;

  // Off-screen Render Targets
  std::unique_ptr<RHI::Texture> fluidDepthTexture;     // R32_FLOAT
  std::unique_ptr<RHI::Texture> fluidThicknessTexture; // R16_FLOAT
  std::unique_ptr<RHI::Texture> internalDepthBuffer;   // D32_FLOAT
  std::unique_ptr<RHI::Texture>
      tempDepthTexture; // Target for the Horizontal pass
  std::unique_ptr<RHI::Texture>
      blurredDepthTexture; // Target for the Vertical pass

  std::unique_ptr<RHI::Pipeline> depthPipeline;
  std::unique_ptr<RHI::Pipeline> thicknessPipeline;
  std::unique_ptr<RHI::Pipeline> blurPipeline;
  std::unique_ptr<RHI::Pipeline> compositePipeline;
};

} // namespace elementalEngine::Renderer