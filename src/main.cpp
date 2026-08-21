#include "CommandList.hpp"
#include "Device.hpp"
#include "PBFSlime.hpp"
#include "SSFRRenderer.hpp" // ✨ Replace PBFSlimeRenderer with this!
#include "Swapchain.hpp"
#include "TerrainPass.hpp"
#include "Window.hpp"
#include "rhi/RHICommon.hpp"
#include <chrono>
#include <cstdlib>
#include <glm/gtc/matrix_inverse.hpp> // ✨ Added for Inverse matrices
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>

using namespace elementalEngine;
using namespace elementalEngine::RHI;

int main() {
  static constexpr int WIDTH{2000};
  static constexpr int HEIGHT{1000};

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
    uint32_t numParticles = 10000;
    Physics::PBFSlime slimeSim(*device, numParticles);
    Renderer::SSFRRenderer ssfrRenderer(*device, swapchain->getWidth(),
                                        swapchain->getHeight());

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

        // Notify the SSFR renderer that the screen changed size!
        ssfrRenderer.onResize(swapchain->getWidth(), swapchain->getHeight());
        continue;
      }

      auto currentTime = std::chrono::high_resolution_clock::now();
      float deltaTime =
          std::chrono::duration<float, std::chrono::seconds::period>(
              currentTime - lastTime)
              .count();

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
      float fixedDt = 0.008f;

      // run slime physics
      slimeSim.simulate(*cmdList, fixedDt, 0.0f, 0.0f, 0.0f);

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

      // PASS 1: RENDER TERRAIN
      cmdList->beginRendering(renderingInfo);
      terrainPass.render(*cmdList, swapchain->getCurrentBackBuffer(),
                         swapchain->getWidth(), swapchain->getHeight());

      // End the terrain pass here so SSFR can do its own multi-pass
      // sequence!
      cmdList->endRendering();

      // ==========================================
      // PASS 2: 3 step SSFR (GBuffer -> Blur -> Composite)
      // ==========================================
      // Extract the matrices we need for Screen-Space logic
      const float *viewMat =
          glm::value_ptr(terrainPass.getCamera().getFrameData().viewMatrix);
      const float *projMat = glm::value_ptr(
          terrainPass.getCamera().getFrameData().projectionMatrix);

      glm::mat4 invView =
          glm::inverse(terrainPass.getCamera().getFrameData().viewMatrix);
      glm::mat4 invProj =
          glm::inverse(terrainPass.getCamera().getFrameData().projectionMatrix);
      const float *invViewMat = glm::value_ptr(invView);
      const float *invProjMat = glm::value_ptr(invProj);

      // A: Render particles to offscreen depth/thickness G-Buffer
      ssfrRenderer.renderGBuffer(*cmdList, slimeSim, viewMat, projMat, 0.35f,
                                 20.0f, 20.0f);

      // B: Melt the depths using the Compute Shader blur
      ssfrRenderer.renderBlur(*cmdList);

      // C: Composite the fluid onto the screen
      float lightDir[3] = {1.5f, 1.5f, 1.5f}; // Matches Terrain lighting
      ssfrRenderer.renderComposite(*cmdList, swapchain->getCurrentBackBuffer(),
                                   terrainPass.getDepthTexture(), invViewMat,
                                   invProjMat, projMat, lightDir);

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
        ssfrRenderer.onResize(swapchain->getWidth(), swapchain->getHeight());
      }
    }

    device->waitIdle();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}