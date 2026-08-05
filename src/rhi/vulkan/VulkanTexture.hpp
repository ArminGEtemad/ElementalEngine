#pragma once

#include "Texture.hpp"
#include "VulkanDevice.hpp"
#include <cstdint>

namespace elementalEngine::RHI {
class VulkanTexture : public Texture {
public:
  VulkanTexture(VulkanDevice &device, uint32_t width, uint32_t height,
                TextureFormat format, TextureUsage usage);
  ~VulkanTexture() override;

  uint32_t getWidth() const override { return width; }
  uint32_t getHeight() const override { return height; }
  TextureFormat getFormat() const override { return format; }
  TextureUsage getUsage() const override { return usage; }

  VkImage getImage() const { return image; }
  VkImageView getImageView() const { return imageView; }
  VkFormat getVkFormat() const { return vkFormat; }

  // helper functions
  static VkFormat mapFormat(TextureFormat format);
  static VkImageUsageFlags mapUsage(TextureUsage usage);

private:
  VulkanDevice &device;
  uint32_t width;
  uint32_t height;
  TextureFormat format;
  TextureUsage usage;

  VkFormat vkFormat;
  VkImage image{VK_NULL_HANDLE};
  VmaAllocation allocation{VK_NULL_HANDLE};
  VkImageView imageView{VK_NULL_HANDLE};
};

} // namespace elementalEngine::RHI