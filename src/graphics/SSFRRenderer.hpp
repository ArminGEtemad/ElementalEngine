#pragma once

#include "CommandList.hpp"
#include "Device.hpp"
#include "PBFSlime.hpp"
#include "Texture.hpp"
#include <array>
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
  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
  SSFRRenderer(RHI::Device &device, uint32_t width, uint32_t height);
  ~SSFRRenderer() = default;

  void onResize(uint32_t width, uint32_t height);

  void renderGBuffer(RHI::CommandList &commandList,
                     const Physics::PBFSlime &slime, const float *viewMatrix,
                     const float *projMatrix, float particleRadius,
                     uint32_t frameIndex, float worldSizeX = 20.0f,
                     float worldSizeZ = 20.0f);

  void renderBlur(RHI::CommandList &cmdList, uint32_t frameIndex);

  void renderComposite(RHI::CommandList &cmdList,
                       RHI::Texture *swapchainTexture,
                       RHI::Texture *terrainDepthTexture,
                       const float *invViewMatrix, const float *invProjMatrix,
                       const float *projMatrix, const float *lightDir,
                       uint32_t frameIndex);

  RHI::Texture *getDepthTexture(uint32_t frameIndex) const {
    return blurredDepthTextures[frameIndex].get();
  }
  RHI::Texture *getThicknessTexture(uint32_t frameIndex) const {
    return fluidThicknessTextures[frameIndex].get();
  }

private:
  void createRenderTargets(uint32_t width, uint32_t height);
  void createPipelines();

  RHI::Device &device;
  uint32_t currentWidth;
  uint32_t currentHeight;

  // Off-screen Render Targets
  std::array<std::unique_ptr<RHI::Texture>, MAX_FRAMES_IN_FLIGHT>
      fluidDepthTextures;
  std::array<std::unique_ptr<RHI::Texture>, MAX_FRAMES_IN_FLIGHT>
      fluidThicknessTextures;
  std::array<std::unique_ptr<RHI::Texture>, MAX_FRAMES_IN_FLIGHT>
      internalDepthBuffers;
  std::array<std::unique_ptr<RHI::Texture>, MAX_FRAMES_IN_FLIGHT>
      tempDepthTextures;
  std::array<std::unique_ptr<RHI::Texture>, MAX_FRAMES_IN_FLIGHT>
      blurredDepthTextures;

  std::unique_ptr<RHI::Pipeline> depthPipeline;
  std::unique_ptr<RHI::Pipeline> thicknessPipeline;
  std::unique_ptr<RHI::Pipeline> blurPipeline;
  std::unique_ptr<RHI::Pipeline> compositePipeline;
};

} // namespace elementalEngine::Renderer