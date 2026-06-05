#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"
#include <stdexcept>
namespace elementalEngine::RHI {
VulkanBuffer::VulkanBuffer(VulkanDevice &device, size_t size, BufferUsage usage,
                           MemoryProperty memoryProperty)
    : device(device), size(size), usage(usage), memoryProperty(memoryProperty) {
  VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;

  VkBufferUsageFlags vkUsage = 0;
  if (usage & BufferUsage::Vertex)
    vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  if (usage & BufferUsage::Index)
    vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  if (usage & BufferUsage::Uniform)
    vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  if (usage & BufferUsage::Storage)
    vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  if (usage & BufferUsage::TransferSrc)
    vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  if (usage & BufferUsage::TransferDst)
    vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  bufferInfo.usage = vkUsage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  if (memoryProperty == MemoryProperty::CPUAccess) {
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  } else {
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  }

  if (vmaCreateBuffer(device.getAllocator(), &bufferInfo, &allocInfo, &buffer,
                      &allocation, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate VMA buffer!");
  }
}

VulkanBuffer::~VulkanBuffer() {
  if (buffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(device.getAllocator(), buffer, allocation);
  }
}

void *VulkanBuffer::map() {
  void *mappedData = nullptr;
  if (vmaMapMemory(device.getAllocator(), allocation, &mappedData) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to map Vulkan buffer memory via VMA!");
  }
  return mappedData;
}

void VulkanBuffer::unmap() {
  vmaUnmapMemory(device.getAllocator(), allocation);
}
} // namespace elementalEngine::RHI