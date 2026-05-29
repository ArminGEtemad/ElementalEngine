#pragma once

#include "Swapchain.hpp"
#include "VulkanDevice.hpp"
#include "Window.hpp"
#include <cstdint>

namespace elementalEngine::RHI {
class VulkanSwapchain : public Swapchain {
public:
  VulkanSwapchain(VulkanDevice &device, WindowHandling &window);
  ~VulkanSwapchain() override;

  void acquireNextImage() override;
  void present() override;
  uint32_t getCurrentFrameIndex() const override { return currentImageIndex; }

private:
  uint32_t currentImageIndex = 0;

  struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  VulkanDevice &device;

  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  std::vector<VkImage> swapchainImages;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtent;
  std::vector<VkImageView> swapchainImageViews;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;

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
};

} // namespace elementalEngine::RHI