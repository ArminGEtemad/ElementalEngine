#pragma once

// header files
#include "Window.hpp"
#include <optional>
#include <vector>

namespace elementalEngine {
struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class ApplicationVK {
public:
  // TODO for now the window is not resizable
  // change it later after triangle is up
  static constexpr int WIDTH{1000};
  static constexpr int HEIGHT{800};

  ApplicationVK();
  ~ApplicationVK();

  // cleaning up
  ApplicationVK(const ApplicationVK &) = delete;
  ApplicationVK &operator=(const ApplicationVK &) = delete;

  // functions
  void run();
  void drawFrame();

private:
  // --- initialization ---
  WindowHandling window{WIDTH, HEIGHT, "Elemental Engine - VK"};
  // - Vulkan Monolith -
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  std::vector<VkImage> swapchainImages;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtent;
  std::vector<VkImageView> swapchainImageViews;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
  VkPipeline graphicsPipeline;
  VkPipelineLayout pipelineLayout;

  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
  static std::vector<char>
  readFile(const std::string &filepath); // helper function to read shader

  void initVulkan();
  void createInstance();
  void setupDebugMessenger();
  void hasInstanceExtension();
  void createSurface();
  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  void createLogicalDevice();
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  void createSwapchain();
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
  void createImageViews();
  void createCommandPool();
  void allocateCommandBuffer();
  void createSyncObjects();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  VkShaderModule createShaderModule(const std::vector<char> &code);
  void createGraphicsPipeline();
};
} // namespace elementalEngine