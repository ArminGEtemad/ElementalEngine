#pragma once

namespace elementalEngine::RHI {
class Swapchain {
public:
  virtual ~Swapchain() = default;

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  virtual void present() = 0;

protected:
  Swapchain() = default;
};
} // namespace elementalEngine::RHI