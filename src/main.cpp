#include "physics/StamFluid.hpp"
#include "rhi/CommandList.hpp"
#include "rhi/Device.hpp"
#include "rhi/Pipeline.hpp"
#include "rhi/RHICommon.hpp"
#include "rhi/Swapchain.hpp"
#include <cstdlib>
#include <iostream>

using namespace elementalEngine;
using namespace elementalEngine::RHI;
using namespace elementalEngine::Physics;

int main() {
  static constexpr int WIDTH{1000};
  static constexpr int HEIGHT{800};
  static constexpr int GRID_WIDTH{256};
  static constexpr int GRID_HEIGHT{256};

  GraphicsAPI selectedBackend;
  int choice;

  std::cout << "-----------------------------------\n";
  std::cout << "   .:: ELEMENTAL ENGINE ::.\n";
  std::cout << "-----------------------------------\n";
  std::cout << "Select Backend:\n";
  std::cout << "  [1] Vulkan 1.3\n";
  std::cout << "  [2] DirectX 12\n";
  std::cout << "Enter your choice: ";

  std::cin >> choice;
  // choice = 2;
  if (choice == 1) {
    selectedBackend = GraphicsAPI::Vulkan;
    std::cout << "Vulkan 1.3 Backend has been selected...\n";
  } else if (choice == 2) {
    selectedBackend = GraphicsAPI::DirectX12;
    std::cout << "DirectX 12 Backend has been selected...\n";
  } else {
    selectedBackend = GraphicsAPI::Vulkan;
    std::cerr << "Invalid choice! Default to Vulkan 1.3...\n";
  }

  try {
    WindowHandling window{WIDTH, HEIGHT, "Elemental Engine"};
    DeviceConfig config{};
    config.enableValidationLayers = true;
    config.enableGPUAssistedValidatioLayer = false;

    std::unique_ptr<Device> device(
        RHIFilter::createDevice(selectedBackend, config, window));
    std::unique_ptr<Swapchain> swapchain = device->createSwapchain(window);

    //  Physics Subsystem
    StamFluid fluidSim(*device, GRID_WIDTH, GRID_HEIGHT);

    PipelineConfig GraphicPipelineConfig;
    GraphicPipelineConfig.bindings = {
        {1, DescriptorType::SampledImage, 1, ShaderStage::Fragment},
        {3, DescriptorType::SampledImage, 1, ShaderStage::Fragment}};
    GraphicPipelineConfig.pushConstants.size = sizeof(SimConfig);
    GraphicPipelineConfig.pushConstants.stage = ShaderStage::Fragment;

    auto graphicsPipeline =
        device->createPipeline("grid_vs", "grid_fs", GraphicPipelineConfig);
    std::unique_ptr<CommandList> commandList = device->createCommandList();

    // initialize textures
    auto setupCmd = device->createCommandList();
    setupCmd->begin();
    fluidSim.init(*setupCmd);
    setupCmd->end();
    device->submit(setupCmd.get(), nullptr);
    device->waitIdle();

    SimConfig simConfigData{};
    simConfigData.gridWidth = GRID_WIDTH;
    simConfigData.gridHeight = GRID_HEIGHT;

    std::cout << "main loop starts now...\n";

    // 4. Main Loop
    while (!window.shouldClose()) {
      glfwPollEvents();
      swapchain->acquireNextImage();

      commandList->begin();

      // --- PHYSICS PASS ---
      fluidSim.simulate(*commandList, 0.016f);

      // --- GRAPHICS PASS ---
      commandList->beginRendering(*swapchain);
      commandList->bindPipeline(*graphicsPipeline);
      commandList->setViewport(0.0f, 0.0f, static_cast<float>(WIDTH),
                               static_cast<float>(HEIGHT));
      commandList->setScissor(0, 0, WIDTH, HEIGHT);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);

      // Grab the textures directly from the physics system!
      commandList->bindTexture(1, fluidSim.getRenderTexture());

      commandList->draw(3, 1, 0, 0); // Fullscreen Triangle
      commandList->endRendering(*swapchain);

      commandList->end();

      device->submit(commandList.get(), swapchain.get());
      swapchain->present();
    }

    device->waitIdle();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}