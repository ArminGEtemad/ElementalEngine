#include "CommandList.hpp"
#include "CubeTestPass.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"
#include "rhi/RHICommon.hpp"
#include <chrono>
#include <cstdlib>
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
    WindowHandling window{WIDTH, HEIGHT, "Elemental Engine - 3D Cube"};
    DeviceConfig config{};
    config.enableValidationLayers = true;
    config.enableGPUAssistedValidatioLayer = false;

    std::unique_ptr<Device> device(RHIFilter::createDevice(config, window));
    std::unique_ptr<Swapchain> swapchain = device->createSwapchain(window);
    std::unique_ptr<CommandList> cmdList = device->createCommandList();

    // make a cube
    Graphics::CubeTestPass cubePass(*device, *swapchain);

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
        cubePass.onResize(swapchain->getWidth(), swapchain->getHeight());
        continue;
      }

      // calculate dt & totalTime
      auto currentTime = std::chrono::high_resolution_clock::now();
      float deltaTime =
          std::chrono::duration<float, std::chrono::seconds::period>(
              currentTime - lastTime)
              .count();
      float totalTime =
          std::chrono::duration<float, std::chrono::seconds::period>(
              currentTime - startTime)
              .count();
      lastTime = currentTime;

      // Update Camera Matrices & GPU Uniform Buffer
      cubePass.update(deltaTime, totalTime);

      // record render comnmand
      cmdList->begin();

      // Runs 3D Render Pass
      cubePass.render(*cmdList, swapchain->getCurrentBackBuffer(),
                      swapchain->getWidth(), swapchain->getHeight());

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
        cubePass.onResize(swapchain->getWidth(), swapchain->getHeight());
      }
    }

    device->waitIdle();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}