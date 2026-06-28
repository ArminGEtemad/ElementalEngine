#include "Texture.hpp"
#include "rhi/CommandList.hpp"
#include "rhi/Device.hpp"
#include "rhi/Pipeline.hpp"
#include "rhi/RHICommon.hpp"
#include "rhi/Swapchain.hpp"
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

    TextureUsage rwTextureUsage = TextureUsage::ShaderResource |
                                  TextureUsage::UnorderedAccess |
                                  TextureUsage::TransferDst;

    // -- ping pong texture
    auto densityPing = device->createTexture(
        GRID_WIDTH, GRID_HEIGHT, TextureFormat::R32_FLOAT, rwTextureUsage);

    auto densityPong = device->createTexture(
        GRID_WIDTH, GRID_HEIGHT, TextureFormat::R32_FLOAT, rwTextureUsage);

    auto velocityPing = device->createTexture(
        GRID_WIDTH, GRID_HEIGHT, TextureFormat::R32G32_FLOAT, rwTextureUsage);
    auto velocityPong = device->createTexture(
        GRID_WIDTH, GRID_HEIGHT, TextureFormat::R32G32_FLOAT, rwTextureUsage);

    auto divergenceTexture = device->createTexture(
        GRID_WIDTH, GRID_HEIGHT, TextureFormat::R32_FLOAT, rwTextureUsage);

    auto pressurePing = device->createTexture(
        GRID_WIDTH, GRID_HEIGHT, TextureFormat::R32_FLOAT, rwTextureUsage);
    auto pressurePong = device->createTexture(
        GRID_WIDTH, GRID_HEIGHT, TextureFormat::R32_FLOAT, rwTextureUsage);

    bool useBufferPingToRead = true;

    auto graphicsPipeline = device->createPipeline("grid_vs", "grid_fs");

    SimConfig simConfigData{};
    simConfigData.gridWidth = GRID_WIDTH;
    simConfigData.gridHeight = GRID_HEIGHT;
    simConfigData.dt = 0.016f; // for 60fps for now
    simConfigData.forceY = 50.0f;

    std::cout << "main loop starts now...\n";
    auto setupCmd = device->createCommandList();
    setupCmd->begin();

    // transition everything to unordered to initialize the writing
    setupCmd->transitionTexture(densityPing.get(), ResourceState::Undefined,
                                ResourceState::UnorderedAccess);
    setupCmd->transitionTexture(densityPong.get(), ResourceState::Undefined,
                                ResourceState::UnorderedAccess);
    setupCmd->transitionTexture(velocityPing.get(), ResourceState::Undefined,
                                ResourceState::UnorderedAccess);
    setupCmd->transitionTexture(velocityPong.get(), ResourceState::Undefined,
                                ResourceState::UnorderedAccess);
    setupCmd->transitionTexture(pressurePing.get(), ResourceState::Undefined,
                                ResourceState::UnorderedAccess);
    setupCmd->transitionTexture(pressurePong.get(), ResourceState::Undefined,
                                ResourceState::UnorderedAccess);
    setupCmd->transitionTexture(divergenceTexture.get(),
                                ResourceState::Undefined,
                                ResourceState::UnorderedAccess);

    // transition everything to shader resource
    setupCmd->transitionTexture(densityPing.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
    setupCmd->transitionTexture(densityPong.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
    setupCmd->transitionTexture(velocityPing.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
    setupCmd->transitionTexture(velocityPong.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
    setupCmd->transitionTexture(pressurePing.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
    setupCmd->transitionTexture(pressurePong.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);
    setupCmd->transitionTexture(divergenceTexture.get(),
                                ResourceState::UnorderedAccess,
                                ResourceState::ShaderResource);

    setupCmd->end();
    device->submit(setupCmd.get(), nullptr);
    device->waitIdle();

    while (!window.shouldClose()) {
      glfwPollEvents();
      swapchain->acquireNextImage();

      Texture *densityRead =
          useBufferPingToRead ? densityPing.get() : densityPong.get();
      Texture *densityWrite =
          useBufferPingToRead ? densityPong.get() : densityPing.get();

      // velocity bounces within the frame. Ping is always the start state.
      Texture *velocityStart = velocityPing.get();
      Texture *velocityAdvected = velocityPong.get();

      commandList->begin();

      // compute
      // ADVECTION PASS
      commandList->transitionTexture(densityWrite,
                                     ResourceState::ShaderResource,
                                     ResourceState::UnorderedAccess);
      commandList->transitionTexture(velocityAdvected,
                                     ResourceState::ShaderResource,
                                     ResourceState::UnorderedAccess);

      commandList->bindPipeline(*advectionPipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);

      commandList->bindTexture(1, densityRead);       // t1: Old Density
      commandList->bindTexture(2, velocityStart);     // t2: Old Velocity
      commandList->bindStorageImage(3, densityWrite); // u3: New Density
      commandList->bindStorageImage(4,
                                    velocityAdvected); // u4: Advected Velocity
      commandList->bindSampler(5);                     // s5: linear sampler
      commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);

      // DIVERGENCE PASS
      commandList->transitionTexture(velocityAdvected,
                                     ResourceState::UnorderedAccess,
                                     ResourceState::ShaderResource);
      commandList->transitionTexture(divergenceTexture.get(),
                                     ResourceState::ShaderResource,
                                     ResourceState::UnorderedAccess);

      commandList->bindPipeline(*divPipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);
      commandList->bindTexture(1, velocityAdvected); // t1: Advected Velocity
      commandList->bindStorageImage(
          3, divergenceTexture.get()); // u3: Divergence Out
      commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);

      // JACOBI SOLVER PASS
      commandList->transitionTexture(divergenceTexture.get(),
                                     ResourceState::UnorderedAccess,
                                     ResourceState::ShaderResource);
      commandList->bindPipeline(*jacobiPipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);
      commandList->bindTexture(2, divergenceTexture.get()); // t2: Divergence In

      bool usePressurePing = true;
      const int JACOBI_ITERATIONS = 20;

      for (int i = 0; i < JACOBI_ITERATIONS; ++i) {
        Texture *pRead =
            usePressurePing ? pressurePing.get() : pressurePong.get();
        Texture *pWrite =
            usePressurePing ? pressurePong.get() : pressurePing.get();

        if (i > 0) {
          commandList->transitionTexture(pRead, ResourceState::UnorderedAccess,
                                         ResourceState::ShaderResource);
        }

        commandList->transitionTexture(pWrite, ResourceState::ShaderResource,
                                       ResourceState::UnorderedAccess);

        commandList->bindTexture(1, pRead);       // t1: Pressure In
        commandList->bindStorageImage(3, pWrite); // u3: Pressure Out
        commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);

        usePressurePing = !usePressurePing;
      }

      // GRADIENT SUBTRACTION PASS
      Texture *finalPressure =
          usePressurePing ? pressurePing.get() : pressurePong.get();

      commandList->transitionTexture(finalPressure,
                                     ResourceState::UnorderedAccess,
                                     ResourceState::ShaderResource);
      commandList->transitionTexture(velocityStart,
                                     ResourceState::ShaderResource,
                                     ResourceState::UnorderedAccess);

      commandList->bindPipeline(*gradPipeline);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);
      commandList->bindTexture(1, finalPressure);    // t1: Solved Pressure
      commandList->bindTexture(2, velocityAdvected); // t2: Advected Velocity
      commandList->bindStorageImage(
          3, velocityStart); // u3: Corrected Velocity Out
      commandList->dispatch(GRID_WIDTH / 8, GRID_HEIGHT / 8, 1);

      // graphic
      // GRAPHICS RENDER PASS
      commandList->transitionTexture(densityWrite,
                                     ResourceState::UnorderedAccess,
                                     ResourceState::ShaderResource);
      commandList->transitionTexture(velocityStart,
                                     ResourceState::UnorderedAccess,
                                     ResourceState::ShaderResource);

      commandList->beginRendering(*swapchain);
      commandList->bindPipeline(*graphicsPipeline);
      commandList->setViewport(0.0f, 0.0f, static_cast<float>(WIDTH),
                               static_cast<float>(HEIGHT));
      commandList->setScissor(0, 0, WIDTH, HEIGHT);
      commandList->pushConstants(0, sizeof(SimConfig), &simConfigData);

      // Ensure the graphics pipeline is reading using a Texture binding
      commandList->bindTexture(1, densityWrite); // t1: Density

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