#pragma once

#include "CommandList.hpp"
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

  void setViewport(float x, float y, float width, float height) override;
  void setScissor(int32_t x, int32_t y, uint32_t width,
                  uint32_t height) override;

  VkCommandBuffer getNativeCommandBuffer() const { return commandBuffer; }

private:
  VulkanDevice &device;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};
} // namespace elementalEngine::RHI