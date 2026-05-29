#include "VulkanSwapchain.hpp"
#include "VulkanDevice.hpp"
#include "Window.hpp"
#include <algorithm>
#include <iostream>

namespace elementalEngine::RHI {

VulkanSwapchain::VulkanSwapchain(VulkanDevice &device, WindowHandling &window)
    : device(device) {
  createSwapchain(window);
  createImageViews();
  createSyncObjects();
}

// cleaning up
VulkanSwapchain::~VulkanSwapchain() {
  vkDestroySemaphore(device.getLogicalDevice(), renderFinishedSemaphore,
                     nullptr);
  vkDestroySemaphore(device.getLogicalDevice(), imageAvailableSemaphore,
                     nullptr);
  vkDestroyFence(device.getLogicalDevice(), inFlightFence, nullptr);

  for (auto imageView : swapchainImageViews) {
    vkDestroyImageView(device.getLogicalDevice(), imageView, nullptr);
  }
  vkDestroySwapchainKHR(device.getLogicalDevice(), swapchain, nullptr);
}

// surface format for swap chain
VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0]; // Fallback to the first available
}

// present mode for swap chain
VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      std::cout << "Present mode: Mailbox"
                << "\n";
      return availablePresentMode;
    }
  }
  std::cout << "Present mode: FIFO"
            << "\n";
  return VK_PRESENT_MODE_FIFO_KHR;
}

// resolution of the images in swap chain
VkExtent2D
VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                                  WindowHandling &window) {

  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {

    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

VulkanSwapchain::SwapchainSupportDetails
VulkanSwapchain::querySwapchainSupport(VkPhysicalDevice physicalDevice) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, device.getSurface(),
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, device.getSurface(),
                                       &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, device.getSurface(),
                                         &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, device.getSurface(),
                                            &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, device.getSurface(), &presentModeCount,
        details.presentModes.data());
  }

  return details;
}

// create swapchain
void VulkanSwapchain::createSwapchain(WindowHandling &window) {
  SwapchainSupportDetails swapchainSupport =
      querySwapchainSupport(device.getPhysicalDevice());

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapchainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapchainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities, window);

  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = device.getSurface();
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {device.getGraphicsQueueFamily(),
                                   device.getPresentQueueFamily()};

  if (device.getGraphicsQueueFamily() != device.getPresentQueueFamily()) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }
  createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device.getLogicalDevice(), &createInfo, nullptr,
                           &swapchain) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &imageCount,
                          nullptr);
  swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &imageCount,
                          swapchainImages.data());

  swapchainImageFormat = surfaceFormat.format;
  swapchainExtent = extent;
}

void VulkanSwapchain::createImageViews() {
  swapchainImageViews.resize(swapchainImages.size());

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapchainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapchainImageFormat;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.getLogicalDevice(), &createInfo, nullptr,
                          &swapchainImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image views!");
    }
  }
}

void VulkanSwapchain::createSyncObjects() {
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first
                                                  // frame doesn't wait forever

  if (vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr,
                        &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr,
                        &renderFinishedSemaphore) != VK_SUCCESS ||
      vkCreateFence(device.getLogicalDevice(), &fenceInfo, nullptr,
                    &inFlightFence) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create synchronization objects!");
  }
}

void VulkanSwapchain::acquireNextImage() {
  vkWaitForFences(device.getLogicalDevice(), 1, &inFlightFence, VK_TRUE,
                  UINT64_MAX);

  vkAcquireNextImageKHR(device.getLogicalDevice(), swapchain, UINT64_MAX,
                        imageAvailableSemaphore, VK_NULL_HANDLE,
                        &currentImageIndex);
  vkResetFences(device.getLogicalDevice(), 1, &inFlightFence);
}

void VulkanSwapchain::present() {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &currentImageIndex;

  if (vkQueuePresentKHR(device.getPresentQueue(), &presentInfo) != VK_SUCCESS) {
    throw std::runtime_error("Swapchain failed to present!");
  }
}
} // namespace elementalEngine::RHI