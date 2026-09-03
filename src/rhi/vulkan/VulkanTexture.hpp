#pragma once

#include "Texture.hpp"
#include "VulkanDevice.hpp"
#include <cstdint>

namespace elementalEngine::RHI {
class VulkanTexture : public Texture {
public:
  // standard constructor owns GPU allocation via VMA
  VulkanTexture(VulkanDevice &device, uint32_t width, uint32_t height,
                TextureFormat format, TextureUsage usage, uint32_t depth = 1);

  // non-owning creating that wraps external vulkan images
  VulkanTexture(VulkanDevice &device, VkImage image, VkImageView imageView,
                uint32_t width, uint32_t height, TextureFormat format,
                TextureUsage usage, uint32_t depth = 1);

  ~VulkanTexture() override;

  uint32_t getWidth() const override { return width; }
  uint32_t getHeight() const override { return height; }
  uint32_t getDepth() const override { return depth; }
  TextureFormat getFormat() const override { return format; }
  TextureUsage getUsage() const override { return usage; }

  VkImage getImage() const { return image; }
  VkImageView getImageView() const { return imageView; }
  VkFormat getVkFormat() const { return vkFormat; }

  // helper functions
  static VkFormat mapFormat(TextureFormat format);
  static TextureFormat reverseMapFormat(VkFormat format);
  static VkImageUsageFlags mapUsage(TextureUsage usage);

private:
  VulkanDevice &device;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  TextureFormat format;
  TextureUsage usage;

  VkFormat vkFormat;
  VkImage image{VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VkImageView imageView{VK_NULL_HANDLE};
  bool ownsResources{true};
};

} // namespace elementalEngine::RHI