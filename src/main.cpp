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
    config.enableGPUAssistedValidatioLayer = false;

    std::unique_ptr<Device> device(
        RHIFilter::createDevice(selectedBackend, config, window));

    std::unique_ptr<Swapchain> swapchain = device->createSwapchain(window);
    // compute shaders
    auto advectionPipeline = device->createComputePipeline("advection");
    auto divPipeline = device->createComputePipeline("divergence");
    auto jacobiPipeline = device->createComputePipeline("jacobi");
    auto gradPipeline = device->createComputePipeline("gradient");

    std::unique_ptr<CommandList> commandList = device->createCommandList();
    // -----------------------------------------------------------------------------
    // Test for compute eulerian grid
    static constexpr uint32_t GRID_WIDTH{256};
    static constexpr uint32_t GRID_HEIGHT{256};
    static constexpr uint32_t CELL_COUNT = GRID_HEIGHT * GRID_WIDTH;

    size_t scalarSize = CELL_COUNT * sizeof(float);
    size_t vectorSize = CELL_COUNT * sizeof(float) * 2; // 2D vector velocity

    // -- ping pong buffer
    auto densityBufferPing = device->createBuffer(
        scalarSize, BufferUsage::Storage, MemoryProperty::GPULocal);
    auto densityBufferPong = device->createBuffer(
        scalarSize, BufferUsage::Storage, MemoryProperty::GPULocal);
    auto velocityBufferPing = device->createBuffer(
        vectorSize, BufferUsage::Storage, MemoryProperty::GPULocal);
    auto velocityBufferPong = device->createBuffer(
        vectorSize, BufferUsage::Storage, MemoryProperty::GPULocal);
    auto divergenceBuffer = device->createBuffer(
        scalarSize, BufferUsage::Storage, MemoryProperty::GPULocal);
    auto pressureBufferPing = device->createBuffer(
        scalarSize, BufferUsage::Storage, MemoryProperty::GPULocal);
    auto pressureBufferPong = device->createBuffer(
        scalarSize, BufferUsage::Storage, MemoryProperty::GPULocal);

    bool useBufferPingToRead = true;

    auto graphicsPipeline = device->createPipeline("grid_vs", "grid_fs");

    SimConfig simConfigData{};
    simConfigData.gridWidth = GRID_WIDTH;
    simConfigData.gridHeight = GRID_HEIGHT;
    simConfigData.dt = 0.016f; // for 60fps for now
    simConfigData.forceY = 50.0f;

    std::cout << "main loop starts now...\n";

    while (!window.shouldClose()) {
      glfwPollEvents();
      swapchain->acquireNextImage();

      Buffer *densityRead = useBufferPingToRead ? densityBufferPing.get()
                                                : densityBufferPong.get();
      Buffer *densityWrite = useBufferPingToRead ? densityBufferPong.get()
                                                 : densityBufferPing.get();

      // velocity bounces within the frame. Ping is always the start state.
      Buffer *velocityStart = velocityBufferPing.get();
      Buffer *velocityAdvected = velocityBufferPong.get();

      commandList->begin();

      // compute
      // compute pass 1 advection
      commandList->transitionBuffer(densityWrite, ResourceState::ShaderResource,
                                    ResourceState::UnorderedAccess);
      commandList->transitionBuffer(velocityAdvected,
                                    ResourceState::ShaderResource,
                                    ResourceState::UnorderedAccess);

      commandList->bindPipeline(*advectionPipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);
      commandList->bindStorageBuffer(1, densityRead);   // t1: Old Density
      commandList->bindStorageBuffer(2, velocityStart); // t2: Old Velocity
      commandList->bindStorageBuffer(3, densityWrite);  // u3: New Density
      commandList->bindStorageBuffer(4,
                                     velocityAdvected); // u4: Advected Velocity
      commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);
      // compute 2 divergence
      // Protect the advected velocity so we can read it, prepare Divergence for
      // writing
      commandList->transitionBuffer(velocityAdvected,
                                    ResourceState::UnorderedAccess,
                                    ResourceState::ShaderResource);
      commandList->transitionBuffer(divergenceBuffer.get(),
                                    ResourceState::ShaderResource,
                                    ResourceState::UnorderedAccess);

      commandList->bindPipeline(*divPipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);
      commandList->bindStorageBuffer(1,
                                     velocityAdvected); // t1: Advected Velocity
      commandList->bindStorageBuffer(
          3, divergenceBuffer.get()); // u3: Divergence Out
      commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);

      // jacobi

      // Protect Divergence so the solver can read it safely
      commandList->transitionBuffer(divergenceBuffer.get(),
                                    ResourceState::UnorderedAccess,
                                    ResourceState::ShaderResource);

      commandList->bindPipeline(*jacobiPipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);
      commandList->bindStorageBuffer(
          2, divergenceBuffer.get()); // t2: Divergence In

      bool usePressurePing = true;
      const int JACOBI_ITERATIONS =
          20; // Increase for stiffer fluid, decrease for performance

      for (int i = 0; i < JACOBI_ITERATIONS; ++i) {
        Buffer *pRead = usePressurePing ? pressureBufferPing.get()
                                        : pressureBufferPong.get();
        Buffer *pWrite = usePressurePing ? pressureBufferPong.get()
                                         : pressureBufferPing.get();

        commandList->transitionBuffer(pRead, ResourceState::UnorderedAccess,
                                      ResourceState::ShaderResource);
        commandList->transitionBuffer(pWrite, ResourceState::ShaderResource,
                                      ResourceState::UnorderedAccess);

        commandList->bindStorageBuffer(1, pRead);  // t1: Pressure In
        commandList->bindStorageBuffer(3, pWrite); // u3: Pressure Out
        commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);

        usePressurePing = !usePressurePing; // Inner loop ping-pong
      }

      // compute pass 4 gradient
      Buffer *finalPressure =
          usePressurePing ? pressureBufferPing.get() : pressureBufferPong.get();
      commandList->transitionBuffer(finalPressure,
                                    ResourceState::UnorderedAccess,
                                    ResourceState::ShaderResource);

      // Prepare velocityStart (Ping) to be overwritten with the final,
      // corrected velocity
      commandList->transitionBuffer(velocityStart,
                                    ResourceState::ShaderResource,
                                    ResourceState::UnorderedAccess);

      commandList->bindPipeline(*gradPipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);
      commandList->bindStorageBuffer(1, finalPressure); // t1: Solved Pressure
      commandList->bindStorageBuffer(2,
                                     velocityAdvected); // t2: Advected Velocity
      commandList->bindStorageBuffer(
          3, velocityStart); // u3: Corrected Velocity Out (Back to Ping!)
      commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);

      // render
      // Transition the density write buffer and velocity buffer so the graphics
      // pipeline can read them
      commandList->transitionBuffer(densityWrite,
                                    ResourceState::UnorderedAccess,
                                    ResourceState::ShaderResource);
      commandList->transitionBuffer(velocityStart,
                                    ResourceState::UnorderedAccess,
                                    ResourceState::ShaderResource);

      commandList->beginRendering(*swapchain);
      commandList->bindPipeline(*graphicsPipeline);
      commandList->setViewport(0.0f, 0.0f, static_cast<float>(WIDTH),
                               static_cast<float>(HEIGHT));
      commandList->setScissor(0, 0, WIDTH, HEIGHT);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);

      // graphics Pipeline reads the exact buffer compute just finished
      // writing to
      commandList->bindStorageBuffer(1, densityWrite);

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