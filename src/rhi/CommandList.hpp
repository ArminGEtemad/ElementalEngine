#pragma once

#include "Buffer.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include "Texture.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace elementalEngine::RHI {
// Everything needed for recoding graphics and compute work into the command
// buffers.
// commands are queued and evaluated upon queue submission.

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

  // Command buffer recording life cycle from beginning the recording and
  // reseting the pool to closing the command list and readying for submission
  virtual void begin() = 0;
  virtual void end() = 0;
  virtual void beginRendering(const RenderingInfo &info) = 0;
  virtual void endRendering() = 0;

  // clearing, copying, and transitioning resource states to prevent race
  // conditions - the ResourceState can be found in the rhicommon.
  virtual void clearBuffer(Buffer *buffer, uint32_t value) = 0;
  virtual void copyBuffer(Buffer *src, Buffer *dst, size_t size,
                          size_t srcOffset = 0, size_t dstOffset = 0) = 0;
  virtual void transitionBuffer(Buffer *buffer, ResourceState to) = 0;
  virtual void transitionTexture(Texture *texture, ResourceState to) = 0;

  // binding functions
  // the bind pipeline function is unified. graphics and compute both use the
  // same pipeline function. Not to be confused the creating pipelines. Creating
  // pipelines is not unified. look into the Device header file!
  //  push constants for low latency updates for small per draw such as camera
  virtual void bindTexture(uint32_t bindingSlot, Texture *texture) = 0;
  virtual void bindStorageImage(uint32_t bindingSlot, Texture *texture) = 0;
  virtual void bindSampler(uint32_t bindingSlot) = 0;
  virtual void bindIndexBuffer(Buffer *buffer, IndexType indexType,
                               size_t offset = 0) = 0;
  virtual void bindPipeline(Pipeline &pipeline) = 0;
  virtual void bindVertexBuffer(Buffer *buffer, size_t stride) = 0;
  virtual void bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) = 0;
  virtual void bindUniformBuffer(uint32_t bindingSlot, Buffer *buffer,
                                 size_t offset = 0, size_t range = 0) = 0;
  virtual void pushConstants(uint32_t offset, uint32_t size, const void *data,
                             ShaderStage stage) = 0;

  // draw/dispatch calls and rasterization
  virtual void setViewport(float x, float y, float width, float height) = 0;
  virtual void setScissor(int32_t x, int32_t y, uint32_t width,
                          uint32_t height) = 0;
  virtual void draw(uint32_t vertexCount, uint32_t instanceCount,
                    uint32_t firstVertex, uint32_t firstInstance) = 0;
  virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                        uint32_t groupCountZ) = 0;
  virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                           uint32_t firstIndex, int32_t vertexOffset,
                           uint32_t firstInstance) = 0;
  virtual void drawIndirect(Buffer *buffer, size_t offset, uint32_t drawCount,
                            uint32_t stride) = 0;
  virtual void drawIndexedIndirect(Buffer *buffer, size_t offset,
                                   uint32_t drawCount, uint32_t stride) = 0;

  // Debug markers needed for profiling tools
  virtual void beginDebugMarker(const char *name,
                                const float color[4] = nullptr) = 0;
  virtual void endDebugMarker() = 0;
  virtual void insertDebugMarker(const char *name,
                                 const float color[4] = nullptr) = 0;

protected:
  CommandList() = default;
};

} // namespace elementalEngine::RHI
