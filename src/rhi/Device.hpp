#pragma once

#include "RHICommon.hpp"
#include "Window.hpp"

namespace elementalEngine::RHI {
class Device {
public:
  virtual ~Device() = default;
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;

  virtual GraphicsAPI getAPI() const = 0;
  virtual void waitIdle() = 0;

protected:
  Device() = default;
};

class RHIFilter {
public:
  static Device *createDevice(GraphicsAPI api, const DeviceConfig &config,
                              WindowHandling &window);
};
} // namespace elementalEngine::RHI