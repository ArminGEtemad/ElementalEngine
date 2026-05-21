#pragma once

// header files
#include "Window.hpp"
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace elementalEngine {
struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};
class Application {
public:
  // TODO for now the window is not resizable
  // change it later after triangle is up
  static constexpr int WIDTH{1000};
  static constexpr int HEIGHT{800};

  Application();
  ~Application();

  // cleaning up
  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  // functions
  void run();

private:
  // --- initialization ---
  WindowHandling window{WIDTH, HEIGHT, "Elemental Engine"};
  // - Vulkan Monolith -
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  void initVulkan();
  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
};
} // namespace elementalEngine