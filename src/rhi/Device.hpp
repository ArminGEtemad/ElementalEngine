#pragma once

#include "RHICommon.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"
#include <memory>

namespace elementalEngine::RHI {

class Swapchain;
class Device {
public:
  virtual ~Device() = default;
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;

  virtual GraphicsAPI getAPI() const = 0;
  virtual void waitIdle() = 0;
  virtual std::unique_ptr<Swapchain>
  createSwapchain(WindowHandling &window) = 0;

protected:
  Device() = default;
};

class RHIFilter {
public:
  static std::unique_ptr<Device> createDevice(GraphicsAPI api,
                                              const DeviceConfig &config,
                                              WindowHandling &window);
};
} // namespace elementalEngine::RHI