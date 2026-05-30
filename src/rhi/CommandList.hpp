#pragma once

#include "Swapchain.hpp"
#include <cstdint>

namespace elementalEngine::RHI {
class CommandList {
public:
  virtual ~CommandList() = default;
  CommandList(const CommandList &) = delete;
  CommandList &operator=(const CommandList &) = delete;

  // adding lifecycle of the command list
  virtual void begin() = 0; // begins recording and resets the allocator/pool
  virtual void
  end() = 0; // closes the command list, readying for the submission

  virtual void beginRendering(Swapchain &swapchain) = 0;
  virtual void endRendering(Swapchain &swapchain) = 0;

  // drawing commands
  virtual void setViewport(float x, float y, float width, float height) = 0;
  virtual void setScissor(int32_t x, int32_t y, uint32_t width,
                          uint32_t height) = 0;

protected:
  CommandList() = default;
};

} // namespace elementalEngine::RHI