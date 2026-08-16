#pragma once

#include "Buffer.hpp"
#include "Camera.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "Swapchain.hpp"
#include "Texture.hpp"
#include <memory>

namespace elementalEngine::Graphics {

class TerrainPass {
public:
  TerrainPass(RHI::Device &device, RHI::Swapchain &swapchain);
  ~TerrainPass() = default;

  // Updates camera matrix transformations
  void update(WindowHandling &window, float deltaTime, float totalTime);

  // Records 3D render pass commands into command list
  void render(RHI::CommandList &commandList, RHI::Texture *targetColorTexture,
              uint32_t width, uint32_t height);

  // Recreates depth texture when window resolution changes
  void onResize(uint32_t newWidth, uint32_t newHeight);

  Core::Camera &getCamera() { return camera; }

  // getter
  // this now communicates with Clavet
  RHI::Texture *getDepthTexture() const { return depthTexture.get(); }

private:
  void createDepthTarget(uint32_t width, uint32_t height);
  void createBuffers();
  void createPipeline();

  uint32_t indexCount{0};

  RHI::Device &device;
  Core::Camera camera;

  std::unique_ptr<RHI::Texture> depthTexture;
  std::unique_ptr<RHI::Buffer> vertexBuffer;
  std::unique_ptr<RHI::Buffer> indexBuffer;
  std::unique_ptr<RHI::Buffer> cameraUniformBuffer;
  std::unique_ptr<RHI::Pipeline> pipeline;
};

} // namespace elementalEngine::Graphics