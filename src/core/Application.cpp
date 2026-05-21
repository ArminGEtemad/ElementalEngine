#include "Application.hpp"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace elementalEngine {
Application::Application(){};
Application::~Application() { vkDestroyInstance(instance, nullptr); };

void Application::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
  }
}

void Application::initVulkan() { createInstance(); }

void Application::createInstance() {
  // App Info
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Elemental Engine Project";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "elementalEngine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  // Instance Info
  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
  createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();

  const std::vector<VkValidationFeatureEnableEXT> validationEnables = {
      VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
      VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};

  VkValidationFeaturesEXT validationFeatures = {};
  validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  validationFeatures.enabledValidationFeatureCount =
      static_cast<uint32_t>(validationEnables.size());
  validationFeatures.pEnabledValidationFeatures = validationEnables.data();

  createInfo.pNext =
      &validationFeatures; // linking validation features to the creation

  // glfw Extensions
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

// apple device portability
#ifdef __APPLE__
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  // create
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance!");
  }
}

} // namespace elementalEngine