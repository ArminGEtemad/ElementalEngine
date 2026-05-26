#pragma once

// add header files
#include "Device.hpp"
#include "RHICommon.hpp"
#include "Window.hpp"
#include <vulkan/vulkan.h>

namespace elementalEngine::RHI {
class VulkanDevice : public Device {
public:
  VulkanDevice(const DeviceConfig &config, WindowHandling &window);
  ~VulkanDevice() override;

  GraphicsAPI getAPI() const override { return GraphicsAPI::Vulkan; }
  void waitIdle() override;
};

} // namespace elementalEngine::RHI