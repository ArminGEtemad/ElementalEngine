#include "rhi/CommandList.hpp"
#include "rhi/Device.hpp"
#include "rhi/Pipeline.hpp"
#include "rhi/RHICommon.hpp"
#include "rhi/Swapchain.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace elementalEngine;
using namespace elementalEngine::RHI;

int main() {
  static constexpr int WIDTH{1000};
  static constexpr int HEIGHT{800};

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
    config.enableGPUAssistedValidatioLayer = true;

    std::unique_ptr<Device> device(
        RHIFilter::createDevice(selectedBackend, config, window));

    std::unique_ptr<Swapchain> swapchain = device->createSwapchain(window);
    std::unique_ptr<CommandList> commandList = device->createCommandList();
    // -----------------------------------------------------------------------------
    // vertex Data for test
    std::vector<RHI::Vertex> vertices = {{{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
                                         {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                         {{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}};
    size_t bufferSize = vertices.size() * sizeof(RHI::Vertex);
    auto vertexBuffer = device->createBuffer(
        bufferSize, RHI::BufferUsage::Vertex, RHI::MemoryProperty::CPUAccess);
    void *mappedMemory = vertexBuffer->map();
    std::memcpy(mappedMemory, vertices.data(), bufferSize);
    vertexBuffer->unmap();
    // -----------------------------------------------------------------------------
    std::unique_ptr<Pipeline> pipeline = device->createPipeline();

    std::cout << "main loop starts now...\n";
    while (!window.shouldClose()) {
      glfwPollEvents();
      swapchain->acquireNextImage();

      commandList->begin();
      commandList->beginRendering(*swapchain);
      commandList->bindPipeline(*pipeline);
      commandList->setViewport(0.0f, 0.0f, static_cast<float>(WIDTH),
                               static_cast<float>(HEIGHT));
      commandList->setScissor(0, 0, WIDTH, HEIGHT);
      commandList->bindVertexBuffer(vertexBuffer.get(), sizeof(RHI::Vertex));
      commandList->draw(3, 1, 0, 0);

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