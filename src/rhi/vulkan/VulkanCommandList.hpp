#pragma once

#include "CommandList.hpp"
#include "Pipeline.hpp"
#include "Swapchain.hpp"
#include "VulkanDevice.hpp"

namespace elementalEngine::RHI {
class VulkanCommandList : public CommandList {
public:
  explicit VulkanCommandList(VulkanDevice &device);
  ~VulkanCommandList();

  void begin() override;
  void end() override;
  void beginRendering(Swapchain &swapchain) override;
  void endRendering(Swapchain &swapchain) override;

  void transitionBuffer(Buffer *buffer, ResourceState from,
                        ResourceState to) override;

  void bindPipeline(Pipeline &pipeline) override;

  void setViewport(float x, float y, float width, float height) override;
  void setScissor(int32_t x, int32_t y, uint32_t width,
                  uint32_t height) override;
  void bindVertexBuffer(Buffer *buffer, size_t stride) override;
  void pushConstants(uint32_t offset, uint32_t size, const void *data) override;
  void bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) override;
  void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                uint32_t groupCountZ) override;
  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
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