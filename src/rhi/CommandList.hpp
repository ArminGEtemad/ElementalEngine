#pragma once

#include "Buffer.hpp"
#include "ComputePipeline.hpp"
#include "Pipeline.hpp"
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

  virtual void bindPipeline(Pipeline &pipeline) = 0;
  virtual void bindVertexBuffer(Buffer *buffer, size_t stride) = 0;

  virtual void bindComputePipeline(ComputePipeline &pipeline) = 0;
  virtual void bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) = 0;

  virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                        uint32_t groupCountZ) = 0;
  virtual void draw(uint32_t vertexCount, uint32_t instanceCount,
                    uint32_t firstVertex, uint32_t firstInstance) = 0;

protected:
  CommandList() = default;
};

} // namespace elementalEngine::RHI