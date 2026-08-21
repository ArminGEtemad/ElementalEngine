#include "VulkanSwapchain.hpp"
#include "VulkanDevice.hpp"
#include "Window.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace elementalEngine::RHI {

VulkanSwapchain::VulkanSwapchain(VulkanDevice &device, WindowHandling &window)
    : device(device) {
  createSwapchain(window);
  createImageViews();
  createSyncObjects();
}

// cleaning up
VulkanSwapchain::~VulkanSwapchain() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device.getLogicalDevice(), imageAvailableSemaphore[i],
                       nullptr);
    vkDestroyFence(device.getLogicalDevice(), inFlightFence[i], nullptr);
  }

  for (size_t i = 0; i < renderFinishedSemaphore.size(); i++) {
    vkDestroySemaphore(device.getLogicalDevice(), renderFinishedSemaphore[i],
                       nullptr);
  }

  cleanupSwapchain();
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

  // enforcing double buffering
  uint32_t imageCount = 2;
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
  backBuffers.reserve(swapchainImages.size());

  TextureFormat engineFormat =
      VulkanTexture::reverseMapFormat(swapchainImageFormat);

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

    // Wrap into VulkanTexture instances
    backBuffers.push_back(std::make_unique<VulkanTexture>(
        device, swapchainImages[i], swapchainImageViews[i],
        swapchainExtent.width, swapchainExtent.height, engineFormat,
        TextureUsage::RenderTarget));
  }
}

void VulkanSwapchain::createSyncObjects() {
  imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFence.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first
                                                  // frame doesn't wait forever

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr,
                          &imageAvailableSemaphore[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr,
                          &renderFinishedSemaphore[i]) != VK_SUCCESS ||
        vkCreateFence(device.getLogicalDevice(), &fenceInfo, nullptr,
                      &inFlightFence[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create sync object for frames.");
    }
  }
}

void VulkanSwapchain::cleanupSwapchain() {
  // clear RHI texture wrappers first to avoid referencing destroyed image views
  backBuffers.clear();

  for (auto imageView : swapchainImageViews) {
    vkDestroyImageView(device.getLogicalDevice(), imageView, nullptr);
  }
  swapchainImageViews.clear();
  if (swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device.getLogicalDevice(), swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
  }
}

void VulkanSwapchain::recreate(WindowHandling &window) {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glfwWaitEvents();
  }
  device.waitIdle();
  cleanupSwapchain();
  createSwapchain(window);
  createImageViews();
}

bool VulkanSwapchain::acquireNextImage() {
  vkWaitForFences(device.getLogicalDevice(), 1, &inFlightFence[currentFrame],
                  VK_TRUE, UINT64_MAX);

  VkResult result =
      vkAcquireNextImageKHR(device.getLogicalDevice(), swapchain, UINT64_MAX,
                            imageAvailableSemaphore[currentFrame],
                            VK_NULL_HANDLE, &currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swapchain image!");
  }

  // if acquire succeeded reset the fence
  vkResetFences(device.getLogicalDevice(), 1, &inFlightFence[currentFrame]);
  return true;
}

bool VulkanSwapchain::present() {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  VkSemaphore signalSemaphore[] = {renderFinishedSemaphore[currentFrame]};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &currentImageIndex;

  VkResult result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  vkQueueWaitIdle(device.getPresentQueue());
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return false; // which has to trigger recreate in the main loop
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Swapchain failed to present!");
  }

  return true;
}
} // namespace elementalEngine::RHI