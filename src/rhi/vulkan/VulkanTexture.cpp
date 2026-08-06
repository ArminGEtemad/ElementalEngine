#include "VulkanTexture.hpp"
#include "Texture.hpp"
#include "VulkanDevice.hpp"
#include <stdexcept>

namespace elementalEngine::RHI {
VulkanTexture::VulkanTexture(VulkanDevice &device, uint32_t width,
                             uint32_t height, TextureFormat format,
                             TextureUsage usage)
    : device(device), width(width), height(height), format(format),
      usage(usage) {

  vkFormat = mapFormat(format);
  VkImageUsageFlags vkUsage = mapUsage(usage);

  bool isDepth = (format == TextureFormat::D32_FLOAT ||
                  format == TextureFormat::D24_UNORM_S8_UINT);

  VkImageAspectFlags aspectFlags =
      isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  if (format == TextureFormat::D24_UNORM_S8_UINT) {
    aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = vkFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = vkUsage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  if (vmaCreateImage(device.getAllocator(), &imageInfo, &allocInfo, &image,
                     &allocation, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate texture via VMA!");
  }

  VkImageViewCreateInfo imageViewInfo{};
  imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewInfo.image = image;
  imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewInfo.format = vkFormat;
  imageViewInfo.subresourceRange.aspectMask = aspectFlags;
  imageViewInfo.subresourceRange.baseMipLevel = 0;
  imageViewInfo.subresourceRange.levelCount = 1;
  imageViewInfo.subresourceRange.baseArrayLayer = 0;
  imageViewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device.getLogicalDevice(), &imageViewInfo, nullptr,
                        &imageView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan Image View!");
  }
}

// Non-owning constructor for external Vulkan Images
VulkanTexture::VulkanTexture(VulkanDevice &device, VkImage image,
                             VkImageView imageView, uint32_t width,
                             uint32_t height, TextureFormat format,
                             TextureUsage usage)
    : device(device), width(width), height(height), format(format),
      usage(usage), image(image), imageView(imageView), ownsResources(false) {
  vkFormat = mapFormat(format);
}

// Destructor
VulkanTexture::~VulkanTexture() {
  if (ownsResources) {
    if (imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(device.getLogicalDevice(), imageView, nullptr);
    }
    if (image != VK_NULL_HANDLE) {
      vmaDestroyImage(device.getAllocator(), image, allocation);
    }
  }
}

VkFormat VulkanTexture::mapFormat(TextureFormat format) {
  switch (format) {
  case TextureFormat::B8G8R8A8_SRGB:
    return VK_FORMAT_B8G8R8A8_SRGB;
  case TextureFormat::R8G8B8A8_UNORM:
    return VK_FORMAT_R8G8B8A8_UNORM;
  case TextureFormat::R32_FLOAT:
    return VK_FORMAT_R32_SFLOAT;
  case TextureFormat::R32G32_FLOAT:
    return VK_FORMAT_R32G32_SFLOAT;
  case TextureFormat::D32_FLOAT:
    return VK_FORMAT_D32_SFLOAT;
  case TextureFormat::D24_UNORM_S8_UINT:
    return VK_FORMAT_D24_UNORM_S8_UINT;
  default:
    throw std::runtime_error("Unsupported Texture format in Vulkan!");
  }
}

TextureFormat VulkanTexture::reverseMapFormat(VkFormat format) {
  switch (format) {
  case VK_FORMAT_B8G8R8A8_SRGB:
    return TextureFormat::B8G8R8A8_SRGB;
  case VK_FORMAT_R8G8B8A8_UNORM:
    return TextureFormat::R8G8B8A8_UNORM;
  case VK_FORMAT_R32_SFLOAT:
    return TextureFormat::R32_FLOAT;
  case VK_FORMAT_R32G32_SFLOAT:
    return TextureFormat::R32G32_FLOAT;
  case VK_FORMAT_D32_SFLOAT:
    return TextureFormat::D32_FLOAT;
  case VK_FORMAT_D24_UNORM_S8_UINT:
    return TextureFormat::D24_UNORM_S8_UINT;
  default:
    throw std::runtime_error("Unsupported VkFormat in reverse mapping!");
  }
}

VkImageUsageFlags VulkanTexture::mapUsage(TextureUsage usage) {
  VkImageUsageFlags flags = 0;
  if (usage & TextureUsage::ShaderResource)
    flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
  if (usage & TextureUsage::UnorderedAccess)
    flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  if (usage & TextureUsage::RenderTarget)
    flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if (usage & TextureUsage::DepthStencilAttachment)
    flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  if (usage & TextureUsage::TransferSrc)
    flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  if (usage & TextureUsage::TransferDst)
    flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  return flags;
}

} // namespace elementalEngine::RHI