#include "VulkanDevice.hpp"
#include "CommandList.hpp"
#include "Pipeline.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanCommandList.hpp"
#include "VulkanComputePipeline.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanSwapchain.hpp"
#include "Window.hpp"
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

namespace elementalEngine::RHI {

std::unique_ptr<Swapchain>
VulkanDevice::createSwapchain(WindowHandling &window) {
  return std::make_unique<VulkanSwapchain>(*this, window);
}

std::unique_ptr<CommandList> VulkanDevice::createCommandList() {
  return std::make_unique<VulkanCommandList>(*this);
}

std::unique_ptr<Pipeline>
VulkanDevice::createPipeline(const std::string &vertexShaderName,
                             const std::string &fragmentShaderName) {
  return std::make_unique<VulkanPipeline>(*this, VK_FORMAT_B8G8R8A8_SRGB,
                                          vertexShaderName, fragmentShaderName);
}

std::unique_ptr<Pipeline>
VulkanDevice::createComputePipeline(const std::string &computeShaderName) {
  return std::make_unique<VulkanComputePipeline>(*this, computeShaderName);
}

std::unique_ptr<Buffer> VulkanDevice::createBuffer(size_t size,
                                                   BufferUsage usage,
                                                   MemoryProperty memory) {
  return std::make_unique<VulkanBuffer>(*this, size, usage, memory);
}

// just for internal linkage
namespace {
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << "\n";

  return VK_FALSE;
}

// For validation
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}
} // namespace

// constructor
VulkanDevice::VulkanDevice(const DeviceConfig &config, WindowHandling &window) {
  createInstance(config);
  if (config.enableValidationLayers) {
    setupDebugMessenger();
  }
  createSurface(window);
  pickPhysicalDevice();
  createLogicalDevice();
  createAllocator();
}

// destructor
VulkanDevice::~VulkanDevice() {
  waitIdle();
  vmaDestroyAllocator(allocator);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  if (debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }
  vkDestroyInstance(instance, nullptr);
}

//
void VulkanDevice::waitIdle() { vkDeviceWaitIdle(device); }

// available instance extension printed pout when creating an instance later on
void VulkanDevice::hasInstanceExtension() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensions.data());
  std::cout << "available extensions:\n";
  for (const auto &extension : extensions) {
    std::cout << '\t' << extension.extensionName << '\n';
  }
}

// create vulkan instance
void VulkanDevice::createInstance(const DeviceConfig &config) {
  // extension support
  hasInstanceExtension();

  // App Info
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Elemental Engine Project";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "elementalEngine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  // Instance Info
  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  VkValidationFeaturesEXT validationFeatures{};
  std::vector<VkValidationFeatureEnableEXT> validationEnables;

  // validation layers and features
  if (config.enableValidationLayers) {
    std::cout << "Vulkan Core Debug Layer Enabled.\n";
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    validationEnables = {
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};

    if (config.enableGPUAssistedValidatioLayer) {
      std::cout << "Vulkan GPU-Based Validation Enabled.\n";
      validationEnables.push_back(
          VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
    }
    validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.enabledValidationFeatureCount =
        static_cast<uint32_t>(validationEnables.size());
    validationFeatures.pEnabledValidationFeatures = validationEnables.data();
    createInfo.pNext = &validationFeatures;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  // glfw Extensions
  uint32_t glfwExtensionCount{0};
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

// create debug messanger
void VulkanDevice::setupDebugMessenger() {
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  // catch warnings and errors (ignore verbose/info spam)
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  // catch all types of issues (General, Validation, Performance)
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr; // Optional

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("Failed to set up debug messenger!");
  }
}

// create surface
void VulkanDevice::createSurface(WindowHandling &window) {
  if (glfwCreateWindowSurface(instance, window.getGLFWwindow(), nullptr,
                              &surface) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create a surface");
  }
}

// helper function to check device extention support needed for swapchain
bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

// helper function to see if the device is suitable
bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  QueueFamilyIndices indices = findQueueFamilies(device);
  bool supportsVulkan13 = deviceProperties.apiVersion >= VK_API_VERSION_1_3;

  // if vulkan 1.3 is not supported just stop
  if (!supportsVulkan13 || !indices.isComplete()) {
    return false;
  }

  bool extensionsSupported = checkDeviceExtensionSupport(device);
  if (!extensionsSupported) {
    return false;
  }

  // GPU type
  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    std::cout << "Selected Discrete GPU: " << deviceProperties.deviceName
              << "\n";
  } else {
    std::cout << "Selected Integrated/Fallback GPU (performance might be "
                 "affected): "
              << deviceProperties.deviceName << "\n";
  }

  return true;
}

// pick physical device
void VulkanDevice::pickPhysicalDevice() {
  uint32_t deviceCount{0};
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support!");
  }
  std::cout << "Device count: " << deviceCount << "\n";
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("Failed to find a suitable GPU");
  }
}

// find queue families
VulkanDevice::QueueFamilyIndices
VulkanDevice::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount{0};
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }
    if (indices.isComplete())
      break;
    i++;
  }

  return indices;
}

// create logical device
void VulkanDevice::createLogicalDevice() {
  this->indices = findQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

  float queuePriority{1.0f};
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // zero base feature
  VkPhysicalDeviceFeatures deviceFeaturs{};

  // Vulkan 1.3
  VkPhysicalDeviceVulkan13Features features13{};
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features13.dynamicRendering = VK_TRUE;
  features13.synchronization2 = VK_TRUE;

  // --- not needed for now but not to forget them later I add them now ---
  VkPhysicalDeviceVulkan12Features features12{};
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12.bufferDeviceAddress = VK_TRUE;
  features12.pNext = &features13;

  VkPhysicalDeviceFeatures2 deviceFeatures2{};
  deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
  deviceFeatures2.pNext = &features12;

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pNext = &deviceFeatures2;
  createInfo.pEnabledFeatures = nullptr;

  // not needed just legacy safe
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = nullptr;

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device!");
  }

  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanDevice::submit(CommandList *commandList, Swapchain *swapchain) {
  auto *vk13CmdList = static_cast<VulkanCommandList *>(commandList);
  VkCommandBuffer commandBuffer = vk13CmdList->getNativeCommandBuffer();

  VkCommandBufferSubmitInfo cmdBufferInfo{};
  cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdBufferInfo.commandBuffer = commandBuffer;

  VkSubmitInfo2 submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submitInfo.commandBufferInfoCount = 1;
  submitInfo.pCommandBufferInfos = &cmdBufferInfo;
  if (swapchain != nullptr) {
    auto *vk13Swapchain = static_cast<VulkanSwapchain *>(swapchain);

    VkSemaphoreSubmitInfo waitSemaphoreInfo{};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = vk13Swapchain->getImageAvailableSemaphore();
    waitSemaphoreInfo.stageMask =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signalSemaphoreInfo{};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.semaphore = vk13Swapchain->getRenderFinishedSemaphore();
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

    // 🟢 Submit with the In-Flight Fence
    if (vkQueueSubmit2(graphicsQueue, 1, &submitInfo,
                       vk13Swapchain->getInFlightFence()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to submit draw command buffer!");
    }
  } else {
    // 🟢 Headless fallback (no semaphores, no fence)
    if (vkQueueSubmit2(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to submit headless command buffer!");
    }
  }
}

void VulkanDevice::createAllocator() {
  VmaAllocatorCreateInfo allocInfo{};
  allocInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  allocInfo.physicalDevice = physicalDevice;
  allocInfo.device = device;
  allocInfo.instance = instance;

  if (vmaCreateAllocator(&allocInfo, &allocator) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create VMA!");
  }
}

} // namespace elementalEngine::RHI