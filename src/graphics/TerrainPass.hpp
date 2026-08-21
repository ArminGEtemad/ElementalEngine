#pragma once

#include "Buffer.hpp"
#include "Camera.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "Swapchain.hpp"
#include "Texture.hpp"
#include <array>
#include <memory>

namespace elementalEngine::Graphics {

class TerrainPass {
public:
  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

  TerrainPass(RHI::Device &device, RHI::Swapchain &swapchain);
  ~TerrainPass() = default;

  // Updates camera matrix transformations
  void update(WindowHandling &window, float deltaTime, float totalTime,
              uint32_t frameIndex);
  // Records 3D render pass commands into command list
  void render(RHI::CommandList &commandList, RHI::Texture *targetColorTexture,
              uint32_t width, uint32_t height, uint32_t frameIndex);

  // Recreates depth texture when window resolution changes
  void onResize(uint32_t newWidth, uint32_t newHeight);

  Core::Camera &getCamera() { return camera; }

  // getter
  // this now communicates with Clavet
  RHI::Texture *getDepthTexture(uint32_t frameIndex) const {
    return depthTextures[frameIndex].get();
  }

private:
  void createDepthTarget(uint32_t width, uint32_t height);
  void createBuffers();
  void createPipeline();

  uint32_t indexCount{0};

  RHI::Device &device;
  Core::Camera camera;

  std::array<std::unique_ptr<RHI::Texture>, MAX_FRAMES_IN_FLIGHT> depthTextures;
  std::unique_ptr<RHI::Buffer> vertexBuffer;
  std::unique_ptr<RHI::Buffer> indexBuffer;
  std::array<std::unique_ptr<RHI::Buffer>, MAX_FRAMES_IN_FLIGHT>
      cameraUniformBuffers;
  std::unique_ptr<RHI::Pipeline> pipeline;
};

} // namespace elementalEngine::Graphics