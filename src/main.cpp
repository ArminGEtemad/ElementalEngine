#include "CommandList.hpp"
#include "Device.hpp"
#include "PBFSlime.hpp"
#include "SSFRRenderer.hpp"
#include "Swapchain.hpp"
#include "TerrainPass.hpp"
#include "Window.hpp"
#include "graphics/MidpointLightningRenderer.hpp"
#include "graphics/StamFluidRenderer.hpp"
#include "physics/StamFluid.hpp"
#include "rhi/RHICommon.hpp"
#include <chrono>
#include <cstdlib>
#include <glm/gtc/type_ptr.hpp>
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
    constexpr int MAX_FRAMES_IN_FLIGHT =
        2; // Matches Swapchain setup hardcoded for now. maybe make a getter?
    std::vector<std::unique_ptr<CommandList>> commandLists;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      commandLists.push_back(device->createCommandList());
    }

    // make the terrain
    Graphics::TerrainPass terrainPass(*device, *swapchain);

    // make slime
    uint32_t numParticles = 10000;
    Physics::PBFSlime slimeSim(*device, numParticles);
    Renderer::SSFRRenderer ssfrRenderer(*device, swapchain->getWidth(),
                                        swapchain->getHeight());

    // make poison gas
    Physics::StamFluid stamSim(*device, 128, 128, 128);
    Renderer::StamFluidRenderer stamRenderer(*device);
    Renderer::LightningRenderer lightningRenderer(*device);

    commandLists[0]->begin();
    stamSim.init(*commandLists[0]);
    commandLists[0]->end();
    device->submit(commandLists[0].get(), nullptr);
    device->waitIdle();

    // time tracking instead of hardcoding dt
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastTime = startTime;

    // lightning targeting system
    glm::vec3 lightningTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    float targetSpeed = 15.0f;
    bool isSpacePressed = false;
    float activeShockForce = 0.0f;

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

      // get the safe cpu frame index
      uint32_t syncFrameIdx = swapchain->getSyncFrameIndex();
      CommandList *cmdList = commandLists[syncFrameIdx].get();

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

      // ====================================================================================

      GLFWwindow *rawWindow = window.getGLFWwindow();

      // TODO: it doesn't feel natural right now! it should be connected to the
      // camera I should do it later
      //  Move Z (Forward/Back)
      if (glfwGetKey(rawWindow, GLFW_KEY_UP) == GLFW_PRESS) {
        lightningTarget.z -= targetSpeed * deltaTime;
      }
      if (glfwGetKey(rawWindow, GLFW_KEY_DOWN) == GLFW_PRESS) {
        lightningTarget.z += targetSpeed * deltaTime;
      }

      // Move X (Left/Right)
      if (glfwGetKey(rawWindow, GLFW_KEY_LEFT) == GLFW_PRESS) {
        lightningTarget.x -= targetSpeed * deltaTime;
      }
      if (glfwGetKey(rawWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        lightningTarget.x += targetSpeed * deltaTime;
      }

      // Keep target within the 20x20 terrain bounds
      lightningTarget.x = glm::clamp(lightningTarget.x, -10.0f, 10.0f);
      lightningTarget.z = glm::clamp(lightningTarget.z, -10.0f, 10.0f);

      // zapping
      if (glfwGetKey(rawWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!isSpacePressed) {
          isSpacePressed = true;
          lightningRenderer.triggerLightning(lightningTarget.x,
                                             lightningTarget.z);
          activeShockForce = 1.0f;
          std::cout << "Zapping at the coordinates: " << lightningTarget.x
                    << ", " << lightningTarget.z << "\n";
        }
      } else {
        isSpacePressed = false; // Reset trigger when button is released
      }

      // ====================================================================================

      // Update Camera Matrices & GPU Uniform Buffer
      terrainPass.update(window, deltaTime, totalTime, syncFrameIdx);

      // record render comnmand
      cmdList->begin();
      float fixedDt = 0.008f;

      // run physics
      float sX = activeShockForce > 0 ? lightningTarget.x * 100.0f : 0.0f;
      float sZ = activeShockForce > 0 ? lightningTarget.z * 100.0f : 0.0f;

      slimeSim.simulate(*cmdList, fixedDt, sX, sZ,
                        activeShockForce > 0 ? 30000.0f : 0.0f);
      stamSim.simulate(
          *cmdList, fixedDt, sX, sZ, activeShockForce > 0 ? 800000.0f : 0.0f,
          slimeSim.getParticleBuffer(), slimeSim.getParticleCount());
      activeShockForce = 0.0f;

      // Transition acquired image to Render Target before drawing
      cmdList->transitionTexture(swapchain->getCurrentBackBuffer(),
                                 RHI::ResourceState::Undefined,
                                 RHI::ResourceState::RenderTarget);

      cmdList->transitionTexture(terrainPass.getDepthTexture(syncFrameIdx),
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
      colorAttachment.clearColor[2] = 0.01f;
      colorAttachment.clearColor[3] = 1.0f;
      renderingInfo.colorAttachments.push_back(colorAttachment);

      RHI::DepthAttachment depthAttachment{};
      depthAttachment.texture = terrainPass.getDepthTexture(syncFrameIdx);
      depthAttachment.clear = true;
      depthAttachment.clearDepth = 1.0f;
      depthAttachment.clearStencil = 0;
      renderingInfo.depthAttachment = depthAttachment;

      // PASS 1: RENDER TERRAIN
      cmdList->beginRendering(renderingInfo);
      terrainPass.render(*cmdList, swapchain->getCurrentBackBuffer(),
                         swapchain->getWidth(), swapchain->getHeight(),
                         syncFrameIdx, lightningTarget);
      glm::vec3 camPos = terrainPass.getCamera().getPosition();
      const float *vpMat =
          glm::value_ptr(terrainPass.getCamera().getFrameData().viewProjection);
      lightningRenderer.update(deltaTime);
      lightningRenderer.draw(*cmdList, vpMat, camPos);

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
                                 syncFrameIdx, 20.0f, 20.0f);

      // B: Melt the depths using the Compute Shader blur
      ssfrRenderer.renderBlur(*cmdList, syncFrameIdx);

      // C: Composite the fluid onto the screen
      float lightDir[3] = {1.5f, 1.5f, 1.5f}; // Matches Terrain lighting
      ssfrRenderer.renderComposite(*cmdList, swapchain->getCurrentBackBuffer(),
                                   terrainPass.getDepthTexture(syncFrameIdx),
                                   invViewMat, invProjMat, projMat, lightDir,
                                   syncFrameIdx);

      cmdList->transitionTexture(terrainPass.getDepthTexture(syncFrameIdx),
                                 RHI::ResourceState::DepthStencilWrite,
                                 RHI::ResourceState::ShaderResource);

      glm::mat4 invViewProj = invView * invProj;
      glm::vec3 cameraPos = terrainPass.getCamera().getPosition();

      RHI::RenderingInfo stamInfo{};
      stamInfo.renderWidth = swapchain->getWidth();
      stamInfo.renderHeight = swapchain->getHeight();

      RHI::RenderPassAttachment stamAtt{};
      stamAtt.texture = swapchain->getCurrentBackBuffer();
      stamAtt.clear = false; // Blend over terrain and SSFR slime
      stamInfo.colorAttachments.push_back(stamAtt);

      cmdList->beginRendering(stamInfo);

      stamRenderer.draw(*cmdList, stamSim, invViewProj, cameraPos,
                        swapchain->getWidth(), swapchain->getHeight(),
                        terrainPass.getDepthTexture(syncFrameIdx));
      cmdList->endRendering();

      // Transition Backbuffer to Present
      cmdList->transitionTexture(swapchain->getCurrentBackBuffer(),
                                 RHI::ResourceState::RenderTarget,
                                 RHI::ResourceState::Present);
      cmdList->end();
      // ==============================================

      device->submit(cmdList, swapchain.get());

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