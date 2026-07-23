#include "physics/PBFSlime.hpp"
#include "physics/StamFluid.hpp"
#include "renderer/MidpointLightningRenderer.hpp"
#include "renderer/PBFSlimeRenderer.hpp"
#include "renderer/StamFluidRenderer.hpp"
#include "rhi/CommandList.hpp"
#include "rhi/Device.hpp"
#include "rhi/RHICommon.hpp"
#include "rhi/Swapchain.hpp"
#include <cstdlib>
#include <iostream>

using namespace elementalEngine;
using namespace elementalEngine::RHI;
using namespace elementalEngine::Physics;
using namespace elementalEngine::Renderer;

// build the projection matrix as combination of a translation and scaling for a
// 2D screen. making the world screen understandable for device that neexs a
// [-1,+1]x[-1,+1] coordinate
void projMatrix(float left, float right, float bottom, float top,
                float *resultMatrix) {
  for (int i = 0; i < 16; ++i)
    resultMatrix[i] = 0.0f;
  resultMatrix[0] = 2.0f / (right - left);
  resultMatrix[5] = 2.0f / (top - bottom);
  resultMatrix[10] = 1.0f;
  resultMatrix[12] = -(right + left) / (right - left);
  resultMatrix[13] = -(bottom + top) / (top - bottom);
  resultMatrix[15] = 1.0f;
}

int main() {
  static constexpr int WIDTH{2000};
  static constexpr int HEIGHT{800};
  static constexpr int GRID_WIDTH{2000};
  static constexpr int GRID_HEIGHT{800};

  GraphicsAPI selectedBackend;
  int choice;

  std::cout << "-----------------------------------\n";
  std::cout << "   .:: ELEMENTAL ENGINE ::.\n";
  std::cout << "-----------------------------------\n";
  std::cout << "Select Backend:\n";
  std::cout << "  [1] Vulkan 1.3\n";
  std::cout << "  [2] DirectX 12\n";
  std::cout << "Enter your choice: ";

  // std::cin >> choice;
  choice = 1; // since the DX12 development is on hold
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
    StamFluidRenderer fluidRenderer(*device);
    PBFSlime slimeSim(*device, 100);
    PBFSlimeRenderer slimeRenderer(*device);
    LightningRenderer lightningRenderer(*device);

    float viewProj[16]; // 4x4 projection matrix initialization
    // Map the screen
    projMatrix(0.0f, static_cast<float>(GRID_WIDTH), 0.0f,
               static_cast<float>(GRID_HEIGHT), viewProj);
    // we have make the graphic pipeline
    std::unique_ptr<CommandList> commandList = device->createCommandList();

    // initialize textures
    auto setupCmd = device->createCommandList();
    setupCmd->begin();
    fluidSim.init(*setupCmd);
    setupCmd->end();
    device->submit(setupCmd.get(), nullptr);
    device->waitIdle();

    std::cout << "main loop starts now...\n";

    bool wasClicked = false; // checking if the lightning is triggered

    while (!window.shouldClose()) {
      glfwPollEvents();
      swapchain->acquireNextImage();

      commandList->begin();

      // --- MOUSE CLICK INPUT ---
      int mouseState =
          glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_LEFT);
      if (mouseState == GLFW_PRESS) {
        if (!wasClicked) {
          double xpos, ypos;
          glfwGetCursorPos(window.getGLFWwindow(), &xpos, &ypos);

          float mappedX = static_cast<float>(xpos);
          float mappedY = static_cast<float>(HEIGHT) - static_cast<float>(ypos);

          lightningRenderer.triggerLightning(mappedX, mappedY);

          wasClicked = true;
        }
      } else if (mouseState == GLFW_RELEASE) {
        wasClicked = false;
      }

      // --- PHYSICS PASS ---
      slimeSim.simulate(*commandList, 0.016f);
      fluidSim.simulate(*commandList, 0.016f, slimeSim.getParticleBuffer(),
                        slimeSim.getParticleCount());

      lightningRenderer.update(0.016f);

      slimeRenderer.drawHeightmap(*commandList, slimeSim, viewProj, 8.0f);

      // --- GRAPHICS PASS ---
      commandList->beginRendering(*swapchain);
      fluidRenderer.draw(*commandList, fluidSim, WIDTH, HEIGHT);
      slimeRenderer.drawComposite(*commandList, WIDTH, HEIGHT);
      lightningRenderer.draw(*commandList, viewProj);
      commandList->endRendering(*swapchain);
      commandList->transitionBuffer(slimeSim.getParticleBuffer(),
                                    ResourceState::ShaderResource,
                                    ResourceState::UnorderedAccess);

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