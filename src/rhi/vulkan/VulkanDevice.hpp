#pragma once

// add header files
#include "Device.hpp"
#include "RHICommon.hpp"
#include "Window.hpp"
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace elementalEngine::RHI {

class VulkanDevice : public Device {
public:
  VulkanDevice(const DeviceConfig &config, WindowHandling &window);
  ~VulkanDevice() override;

  GraphicsAPI getAPI() const override { return GraphicsAPI::Vulkan; }
  void waitIdle() override;

  std::unique_ptr<Swapchain> createSwapchain(WindowHandling &window) override;

  // getter functions
  VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
  VkSurfaceKHR getSurface() const { return surface; }
  VkDevice getLogicalDevice() const { return device; }
  uint32_t getGraphicsQueueFamily() const {
    return indices.graphicsFamily.value();
  }
  uint32_t getPresentQueueFamily() const {
    return indices.presentFamily.value();
  }
  VkQueue getPresentQueue() const { return presentQueue; };

private:
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  QueueFamilyIndices indices;

  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  // functions
  void createInstance(const DeviceConfig &config);
  void setupDebugMessenger();
  void createSurface(WindowHandling &window);
  void pickPhysicalDevice();
  void createLogicalDevice();

  // helper function
  void hasInstanceExtension();
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
};

} // namespace elementalEngine::RHI