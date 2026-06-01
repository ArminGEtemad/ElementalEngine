#pragma once
#include <cstdint>

namespace elementalEngine::RHI {
class Swapchain {
public:
  virtual ~Swapchain() = default;

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  virtual void present() = 0;
  virtual void acquireNextImage() = 0;
  virtual uint32_t getCurrentFrameIndex() const = 0;

protected:
  Swapchain() = default;
};
} // namespace elementalEngine::RHI