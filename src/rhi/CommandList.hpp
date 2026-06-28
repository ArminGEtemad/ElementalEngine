#pragma once

#include "Buffer.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include "Swapchain.hpp"
#include "Texture.hpp"
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

  // preventing race condition transitions buffer from - to
  virtual void transitionBuffer(Buffer *buffer, ResourceState from,
                                ResourceState to) = 0;
  virtual void transitionTexture(Texture *texture, ResourceState from,
                                 ResourceState to) = 0;

  // pushing images instead of raw buffers
  virtual void bindTexture(uint32_t bindingSlot, Texture *texture) = 0;
  virtual void bindStorageImage(uint32_t bindingSlot, Texture *texture) = 0;
  virtual void bindSampler(uint32_t bindingSlot) = 0;

  // unified pipeline compute and graphicss
  virtual void bindPipeline(Pipeline &pipeline) = 0;

  // drawing commands
  virtual void setViewport(float x, float y, float width, float height) = 0;
  virtual void setScissor(int32_t x, int32_t y, uint32_t width,
                          uint32_t height) = 0;
  virtual void bindVertexBuffer(Buffer *buffer, size_t stride) = 0;
  virtual void draw(uint32_t vertexCount, uint32_t instanceCount,
                    uint32_t firstVertex, uint32_t firstInstance) = 0;

  // compute commands
  virtual void bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) = 0;
  virtual void pushConstants(uint32_t offset, uint32_t size,
                             const void *data) = 0;
  virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                        uint32_t groupCountZ) = 0;

protected:
  CommandList() = default;
};

} // namespace elementalEngine::RHI