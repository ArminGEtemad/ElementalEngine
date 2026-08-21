#pragma once

#include "Swapchain.hpp"
#include "VulkanDevice.hpp"
#include "VulkanTexture.hpp"
#include "Window.hpp"
#include <cstdint>

namespace elementalEngine::RHI {
class VulkanSwapchain : public Swapchain {
public:
  // enforcing double buffering
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
  VulkanSwapchain(VulkanDevice &device, WindowHandling &window);
  ~VulkanSwapchain() override;

  bool present() override;
  bool acquireNextImage() override;
  void recreate(WindowHandling &window) override;
  uint32_t getCurrentFrameIndex() const override { return currentImageIndex; }
  uint32_t getSyncFrameIndex() const override { return currentFrame; }
  uint32_t getWidth() const override { return swapchainExtent.width; }
  uint32_t getHeight() const override { return swapchainExtent.height; }
  Texture *getCurrentBackBuffer() override {
    return backBuffers[currentImageIndex].get();
  }

  VkImage getImage(uint32_t index) const { return swapchainImages[index]; }
  VkImageView getImageView(uint32_t index) const {
    return swapchainImageViews[index];
  }
  VkExtent2D getExtent() const { return swapchainExtent; }
  VkSemaphore getImageAvailableSemaphore() const {
    return imageAvailableSemaphore[currentFrame];
  }
  VkSemaphore getRenderFinishedSemaphore() const {
    return renderFinishedSemaphore[currentImageIndex];
  }
  VkFence getInFlightFence() const { return inFlightFence[currentFrame]; }

private:
  uint32_t currentFrame = 0;
  uint32_t currentImageIndex = 0;

  struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  VulkanDevice &device;

  VkSwapchainKHR swapchain{VK_NULL_HANDLE};
  std::vector<VkImage> swapchainImages;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtent;
  std::vector<VkImageView> swapchainImageViews;
  std::vector<std::unique_ptr<VulkanTexture>> backBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphore;
  std::vector<VkSemaphore> renderFinishedSemaphore;
  std::vector<VkFence> inFlightFence;

  // choose the surface format needed for the swapchain
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  // choose the present mode needed for the swapchain
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  // resolution for the images waiting to be presented in the swapchain
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                              WindowHandling &window);
  SwapchainSupportDetails
  querySwapchainSupport(VkPhysicalDevice physicalDevice);
  void createSwapchain(WindowHandling &window);
  void createImageViews();
  void createSyncObjects();
  void cleanupSwapchain();
};

} // namespace elementalEngine::RHI