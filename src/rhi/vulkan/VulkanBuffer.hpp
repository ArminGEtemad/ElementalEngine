#pragma once

#include "Buffer.hpp"
#include "RHICommon.hpp"
#include "VulkanDevice.hpp"
#include <cstddef>

namespace elementalEngine::RHI {
class VulkanDevice;
class VulkanBuffer : public Buffer {
public:
  VulkanBuffer(VulkanDevice &device, size_t size, BufferUsage usage,
               MemoryProperty memoryProperty);
  ~VulkanBuffer() override;

  size_t getSize() const override { return size; };
  BufferUsage getBufferUsage() const override { return usage; }
  MemoryProperty getMemoryProperty() const override { return memoryProperty; }
  VkBuffer getVkBuffer() const { return buffer; }
  void *map() override;
  void unmap() override;

private:
  VulkanDevice &device;
  size_t size;
  BufferUsage usage;
  MemoryProperty memoryProperty;

  VkBuffer buffer{VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
};
} // namespace elementalEngine::RHI