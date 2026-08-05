#pragma once

#include "Buffer.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include "Swapchain.hpp"
#include "Texture.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace elementalEngine::RHI {
struct RenderPassAttachment {
  Texture *texture = nullptr;
  bool clear = true;
  float clearColor[4] = {0.01f, 0.01f, 0.1f, 1.0f};
};

struct DepthAttachment {
  Texture *texture = nullptr;
  bool clear = true;
  float clearDepth = 1.0f;
  uint32_t clearStencil = 0;
};

struct RenderingInfo {
  uint32_t renderWidth = 0;
  uint32_t renderHeight = 0;
  std::vector<RenderPassAttachment> colorAttachments;
  std::optional<DepthAttachment> depthAttachment;
};

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
  virtual void beginRendering(Texture *renderTarget) = 0;
  virtual void beginRendering(const RenderingInfo &info) = 0;

  virtual void endRendering(Swapchain &swapchain) = 0;
  virtual void endRendering(Texture *renderTarget) = 0;
  virtual void endRendering() = 0;

  virtual void clearBuffer(Buffer *buffer, uint32_t value) = 0;

  // preventing race condition transitions buffer from - to
  virtual void transitionBuffer(Buffer *buffer, ResourceState from,
                                ResourceState to) = 0;
  virtual void transitionTexture(Texture *texture, ResourceState from,
                                 ResourceState to) = 0;

  // pushing images instead of raw buffers
  virtual void bindTexture(uint32_t bindingSlot, Texture *texture) = 0;
  virtual void bindStorageImage(uint32_t bindingSlot, Texture *texture) = 0;
  virtual void bindSampler(uint32_t bindingSlot) = 0;
  virtual void bindIndexBuffer(Buffer *buffer, IndexType indexType,
                               size_t offset = 0) = 0;

  // unified pipeline compute and graphicss
  virtual void bindPipeline(Pipeline &pipeline) = 0;

  // drawing commands
  virtual void setViewport(float x, float y, float width, float height) = 0;
  virtual void setScissor(int32_t x, int32_t y, uint32_t width,
                          uint32_t height) = 0;
  virtual void bindVertexBuffer(Buffer *buffer, size_t stride) = 0;
  virtual void draw(uint32_t vertexCount, uint32_t instanceCount,
                    uint32_t firstVertex, uint32_t firstInstance) = 0;
  virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                           uint32_t firstIndex, int32_t vertexOffset,
                           uint32_t firstInstance) = 0;

  // compute commands
  virtual void bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) = 0;
  virtual void pushConstants(uint32_t offset, uint32_t size, const void *data,
                             ShaderStage stage) = 0;
  virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                        uint32_t groupCountZ) = 0;

protected:
  CommandList() = default;
};

} // namespace elementalEngine::RHI