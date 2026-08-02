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
  static constexpr int WIDTH{2000};
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
      swapchain->acquireNextImage();
      cmdList->begin();
      cmdList->beginRendering(*swapchain);

      cmdList->setViewport(0.0f, 0.0f, static_cast<float>(WIDTH),
                           static_cast<float>(HEIGHT));
      cmdList->setScissor(0, 0, WIDTH, HEIGHT);

      cmdList->endRendering(*swapchain);
      cmdList->end();

      device->submit(cmdList.get(), swapchain.get());
      swapchain->present();
    }

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}