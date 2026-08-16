#include "CommandList.hpp"
#include "Device.hpp"
#include "PBFSlime.hpp"
#include "PBFSlimeRenderer.hpp"
#include "Swapchain.hpp"
#include "TerrainPass.hpp"
#include "Window.hpp"
#include "rhi/RHICommon.hpp"
#include <chrono>
#include <cstdlib>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>

using namespace elementalEngine;
using namespace elementalEngine::RHI;

int main() {
  static constexpr int WIDTH{800};
  static constexpr int HEIGHT{800};

  std::cout << "-----------------------------------\n";
  std::cout << "   .:: ELEMENTAL ENGINE ::.\n";
  std::cout << "-----------------------------------\n";

  try {
    WindowHandling window{WIDTH, HEIGHT, "Elemental Engine"};
    DeviceConfig config{};
    config.enableValidationLayers = true;
    config.enableGPUAssistedValidatioLayer = false;

    std::unique_ptr<Device> device(RHIFilter::createDevice(config, window));
    std::unique_ptr<Swapchain> swapchain = device->createSwapchain(window);
    std::unique_ptr<CommandList> cmdList = device->createCommandList();

    // make the terrain
    Graphics::TerrainPass terrainPass(*device, *swapchain);

    // make slime
    uint32_t numParticles = 300;
    Physics::PBFSlime slimeSim(*device, numParticles);
    Renderer::PBFSlimeRenderer slimeRenderer(*device);

    // time tracking instead of hardcoding dt
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastTime = startTime;

    std::cout << "main loop starts now...\n";

    while (!window.shouldClose()) {
      glfwPollEvents();

      // skip frame generation if window is minimized
      if (window.isMinimized()) {
        continue;
      }

      // Handle window resize & swapchain image acquisition
      if (window.isResized() || !swapchain->acquireNextImage()) {
        window.resetResizedFlag();
        swapchain->recreate(window);
        terrainPass.onResize(swapchain->getWidth(), swapchain->getHeight());
        continue;
      }

      // calculate dt & totalTime
      // now there is a problem I realized when changind the window size. maybe
      // the dt has to be hardcoded so that we are actually not dependent on the
      // rate? because soe times the visuals can run faster and sometimes
      // slower?
      auto currentTime = std::chrono::high_resolution_clock::now();
      float deltaTime =
          std::chrono::duration<float, std::chrono::seconds::period>(
              currentTime - lastTime)
              .count();
      deltaTime *= 2.0f;
      // changing the window size and moving the window broke the physics. this
      // is just a safety net. We won't have this problem if the dt is
      // hardcoded.
      if (deltaTime > 0.05f) {
        deltaTime = 0.05f;
      }
      float totalTime =
          std::chrono::duration<float, std::chrono::seconds::period>(
              currentTime - startTime)
              .count();
      lastTime = currentTime;

      // Update Camera Matrices & GPU Uniform Buffer
      terrainPass.update(window, deltaTime, totalTime);

      // record render comnmand
      cmdList->begin();

      // run slime physics
      slimeSim.simulate(*cmdList, deltaTime, 0.0f, 0.0f, 0.0f);

      // Transition acquired image to Render Target before drawing
      cmdList->transitionTexture(swapchain->getCurrentBackBuffer(),
                                 RHI::ResourceState::Undefined,
                                 RHI::ResourceState::RenderTarget);

      cmdList->transitionTexture(terrainPass.getDepthTexture(),
                                 RHI::ResourceState::Undefined,
                                 RHI::ResourceState::DepthStencilWrite);
      RHI::RenderingInfo renderingInfo{};
      renderingInfo.renderWidth = swapchain->getWidth();
      renderingInfo.renderHeight = swapchain->getHeight();

      RHI::RenderPassAttachment colorAttachment{};
      colorAttachment.texture = swapchain->getCurrentBackBuffer();
      colorAttachment.clear = true;
      colorAttachment.clearColor[0] = 0.01f;
      colorAttachment.clearColor[1] = 0.01f;
      colorAttachment.clearColor[2] = 0.1f;
      colorAttachment.clearColor[3] = 1.0f;
      renderingInfo.colorAttachments.push_back(colorAttachment);

      RHI::DepthAttachment depthAttachment{};
      depthAttachment.texture = terrainPass.getDepthTexture();
      depthAttachment.clear = true;
      depthAttachment.clearDepth = 1.0f;
      depthAttachment.clearStencil = 0;
      renderingInfo.depthAttachment = depthAttachment;

      cmdList->beginRendering(renderingInfo);
      // Runs 3D Render Pass
      terrainPass.render(*cmdList, swapchain->getCurrentBackBuffer(),
                         swapchain->getWidth(), swapchain->getHeight());

      const float *viewProj =
          glm::value_ptr(terrainPass.getCamera().getFrameData().viewProjection);

      slimeRenderer.render3D(*cmdList, swapchain->getWidth(),
                             swapchain->getHeight(), slimeSim, viewProj, 0.1f);
      cmdList->endRendering();
      // Transition Backbuffer to Present
      cmdList->transitionTexture(swapchain->getCurrentBackBuffer(),
                                 RHI::ResourceState::RenderTarget,
                                 RHI::ResourceState::Present);
      cmdList->end();

      // submit Command Buffer to GPU
      device->submit(cmdList.get(), swapchain.get());

      // Present Frame
      if (!swapchain->present() || window.isResized()) {
        window.resetResizedFlag();
        swapchain->recreate(window);
        terrainPass.onResize(swapchain->getWidth(), swapchain->getHeight());
      }
    }

    device->waitIdle();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}