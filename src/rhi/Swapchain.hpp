#pragma once
#include "Texture.hpp"
#include "Window.hpp"
#include <cstdint>

namespace elementalEngine::RHI {
class Swapchain {
public:
  virtual ~Swapchain() = default;

  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;

  virtual bool present() = 0;
  virtual bool acquireNextImage() = 0;
  virtual void recreate(WindowHandling &window) = 0;

  virtual uint32_t getCurrentFrameIndex() const = 0;
  virtual uint32_t getSyncFrameIndex() const = 0;
  virtual uint32_t getWidth() const = 0;
  virtual uint32_t getHeight() const = 0;

  virtual Texture *getCurrentBackBuffer() = 0;

protected:
  Swapchain() = default;
};
} // namespace elementalEngine::RHI