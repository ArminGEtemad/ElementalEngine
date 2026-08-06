#pragma once

#include "CommandList.hpp"
#include "Pipeline.hpp"
#include "Texture.hpp"
#include "VulkanDevice.hpp"

namespace elementalEngine::RHI {
class VulkanCommandList : public CommandList {
public:
  explicit VulkanCommandList(VulkanDevice &device);
  ~VulkanCommandList();

  void begin() override;
  void end() override;

  void beginRendering(const RenderingInfo &info) override;
  void endRendering() override;

  void clearBuffer(Buffer *buffer, uint32_t value) override;

  void transitionBuffer(Buffer *buffer, ResourceState from,
                        ResourceState to) override;
  void transitionTexture(Texture *texture, ResourceState from,
                         ResourceState to) override;

  void bindTexture(uint32_t bindingSlot, Texture *texture) override;
  void bindStorageImage(uint32_t bindingSlot, Texture *texture) override;
  void bindSampler(uint32_t bindingSlot) override;
  void bindIndexBuffer(Buffer *buffer, IndexType indexType,
                       size_t offset = 0) override;

  void bindPipeline(Pipeline &pipeline) override;

  void setViewport(float x, float y, float width, float height) override;
  void setScissor(int32_t x, int32_t y, uint32_t width,
                  uint32_t height) override;

  void bindVertexBuffer(Buffer *buffer, size_t stride) override;
  void pushConstants(uint32_t offset, uint32_t size, const void *data,
                     ShaderStage stage) override;
  void bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) override;
  void bindUniformBuffer(uint32_t bindingSlot, Buffer *buffer,
                         size_t offset = 0, size_t range = 0) override;
  void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                uint32_t groupCountZ) override;
  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance) override;
  void drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                   uint32_t firstIndex, int32_t vertexOffset,
                   uint32_t firstInstance) override;

  VkCommandBuffer getNativeCommandBuffer() const { return commandBuffer; }

private:
  VulkanDevice &device;
  VkCommandPool commandPool{VK_NULL_HANDLE};
  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  VkPipelineLayout computePiplineLayout{VK_NULL_HANDLE};
  VkPipelineLayout graphicsPiplineLayout{VK_NULL_HANDLE};
  Pipeline *currentPipeline{nullptr};
};
} // namespace elementalEngine::RHI