#include "rhi/CommandList.hpp"
#include "rhi/Device.hpp"
#include "rhi/Pipeline.hpp"
#include "rhi/RHICommon.hpp"
#include "rhi/Swapchain.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

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
    std::unique_ptr<Pipeline> computePipeline = device->createComputePipeline();
    std::unique_ptr<CommandList> commandList = device->createCommandList();
    // -----------------------------------------------------------------------------
    // Test for compute eulerian grid
    static constexpr uint32_t GRID_WIDTH{256};
    static constexpr uint32_t GRID_HEIGHT{256};
    static constexpr uint32_t CELL_COUNT = GRID_HEIGHT * GRID_WIDTH;

    size_t densitySize = CELL_COUNT * sizeof(float);
    size_t velocitySize = CELL_COUNT * sizeof(float) * 2; // 2D vector velocity

    // -- ping pong buffer
    auto densityBufferPing = device->createBuffer(
        densitySize, BufferUsage::Storage, MemoryProperty::GPULocal);
    auto densityBufferPong = device->createBuffer(
        densitySize, BufferUsage::Storage, MemoryProperty::GPULocal);

    auto velocityBufferPing = device->createBuffer(
        velocitySize, BufferUsage::Storage, MemoryProperty::GPULocal);
    auto velocityBufferPong = device->createBuffer(
        velocitySize, BufferUsage::Storage, MemoryProperty::GPULocal);

    bool useBufferPingToRead = true;

    std::unique_ptr<Pipeline> pipeline = device->createPipeline();

    SimConfig simConfigData{};
    simConfigData.gridWidth = GRID_WIDTH;
    simConfigData.gridHeight = GRID_HEIGHT;
    simConfigData.dt = 0.016f; // for 60fps for now
    simConfigData.pad_0 = 0.0f;

    std::cout << "main loop starts now...\n";

    while (!window.shouldClose()) {
      glfwPollEvents();
      swapchain->acquireNextImage();

      Buffer *readBuffer = useBufferPingToRead ? densityBufferPing.get()
                                               : densityBufferPong.get();
      Buffer *writeBuffer = useBufferPingToRead ? densityBufferPong.get()
                                                : densityBufferPing.get();

      commandList->begin();
      // transition the write buffer from a read state (last frame) to a write
      // state (this frame)
      commandList->transitionBuffer(writeBuffer, ResourceState::ShaderResource,
                                    ResourceState::UnorderedAccess);

      // compute
      commandList->bindPipeline(*computePipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);
      commandList->bindStorageBuffer(1, readBuffer);  // t1
      commandList->bindStorageBuffer(3, writeBuffer); // u3
      commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);

      // transition the write buffer so the graphics pipeline can safely read it
      commandList->transitionBuffer(writeBuffer, ResourceState::UnorderedAccess,
                                    ResourceState::ShaderResource);

      // render
      commandList->beginRendering(*swapchain);
      commandList->bindPipeline(*pipeline);
      commandList->setViewport(0.0f, 0.0f, static_cast<float>(WIDTH),
                               static_cast<float>(HEIGHT));
      commandList->setScissor(0, 0, WIDTH, HEIGHT);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);

      // graphics Pipeline reads the exact buffer compute just finished
      // writing to
      commandList->bindStorageBuffer(1, writeBuffer);

      commandList->draw(3, 1, 0, 0);
      commandList->endRendering(*swapchain);

      commandList->end();

      device->submit(commandList.get(), swapchain.get());
      swapchain->present();
      // Ping to Pong and pong to pind for the next frame
      useBufferPingToRead = !useBufferPingToRead;
    }
    device->waitIdle();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}