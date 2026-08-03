#include "CommandList.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"
#include "rhi/RHICommon.hpp"
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
    WindowHandling window{WIDTH, HEIGHT, "Elemental Engine"};
    DeviceConfig config{};
    config.enableValidationLayers = true;
    config.enableGPUAssistedValidatioLayer = false;
    std::unique_ptr<Device> device(RHIFilter::createDevice(config, window));
    std::unique_ptr<Swapchain> swapchain = device->createSwapchain(window);
    std::unique_ptr<CommandList> cmdList = device->createCommandList();

    std::cout << "main loop starts now...\n";

    while (!window.shouldClose()) {
      glfwPollEvents();

      // skip frame generation if minimized
      if (window.isMinimized()) {
        continue;
      }

      // check window resize
      if (window.isResized() || !swapchain->acquireNextImage()) {
        window.resetResizedFlag();
        swapchain->recreate(window);
        continue;
      }

      cmdList->begin();
      cmdList->beginRendering(*swapchain);

      // current swapchain dimensions
      const float currentWidth = static_cast<float>(swapchain->getWidth());
      const float currentHeight = static_cast<float>(swapchain->getHeight());

      cmdList->setViewport(0.0f, 0.0f, currentWidth, currentHeight);
      cmdList->setScissor(0, 0, swapchain->getWidth(), swapchain->getHeight());

      cmdList->endRendering(*swapchain);
      cmdList->end();

      device->submit(cmdList.get(), swapchain.get());

      // present frame and recreate swapchain if suboptimal or out of date
      if (!swapchain->present() || window.isResized()) {
        window.resetResizedFlag();
        swapchain->recreate(window);
      }
    }

    device->waitIdle();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}