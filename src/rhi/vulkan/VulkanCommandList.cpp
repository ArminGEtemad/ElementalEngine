#include "VulkanCommandList.hpp"
#include "Pipeline.hpp"
#include "Swapchain.hpp"
#include "Texture.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanComputePipeline.hpp"
#include "VulkanDevice.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanTexture.hpp"
#include <cstdint>
#include <stdexcept>

namespace elementalEngine::RHI {

static VkShaderStageFlags mapShaderStage(ShaderStage stage) {
  VkShaderStageFlags flags = 0;
  if (stage & ShaderStage::Vertex)
    flags |= VK_SHADER_STAGE_VERTEX_BIT;
  if (stage & ShaderStage::Fragment)
    flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
  if (stage & ShaderStage::Compute)
    flags |= VK_SHADER_STAGE_COMPUTE_BIT;
  return flags;
}

VulkanCommandList::VulkanCommandList(VulkanDevice &device) : device(device) {
  // create command pool
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  poolInfo.queueFamilyIndex = device.getGraphicsQueueFamily();

  if (vkCreateCommandPool(device.getLogicalDevice(), &poolInfo, nullptr,
                          &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool!");
  }

  // allocate command buffer
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(device.getLogicalDevice(), &allocInfo,
                               &commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers!");
  }
}

// cleaning up
VulkanCommandList::~VulkanCommandList() {
  vkDestroyCommandPool(device.getLogicalDevice(), commandPool, nullptr);
}

// begin / reset
void VulkanCommandList::begin() {
  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("Failed to begin recording command buffer!");
  }
}

// end
void VulkanCommandList::end() {
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to record command buffer!");
  }
}

void VulkanCommandList::beginRendering(Swapchain &swapchain) {
  auto &vk13Swapchain = static_cast<VulkanSwapchain &>(swapchain);
  uint32_t frameIndex = vk13Swapchain.getCurrentFrameIndex();
  VkImage image = vk13Swapchain.getImage(frameIndex);
  VkImageView imageView = vk13Swapchain.getImageView(frameIndex);

  VkImageMemoryBarrier2 imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  imageBarrier.srcAccessMask = 0;
  imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  imageBarrier.image = image;
  imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.levelCount = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &imageBarrier;

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);

  VkClearValue clearColor = {{{0.01f, 0.01f, 0.1f, 1.0f}}};

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = imageView;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue = clearColor;

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = vk13Swapchain.getExtent();
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;

  vkCmdBeginRendering(commandBuffer, &renderingInfo);
}
void VulkanCommandList::beginRendering(Texture *renderTarget) {
  auto *vk13Texture = static_cast<VulkanTexture *>(renderTarget);
  VkImage image = vk13Texture->getImage();
  VkImageView imageView = vk13Texture->getImageView();

  VkImageMemoryBarrier2 imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  imageBarrier.srcAccessMask = 0;
  imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  imageBarrier.image = image;
  imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.levelCount = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &imageBarrier;

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);

  // For a heightmap, we clear with 0.0f (representing 0 height)
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 0.0f}}};

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = imageView;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue = clearColor;

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = {vk13Texture->getWidth(),
                                     vk13Texture->getHeight()};
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;

  vkCmdBeginRendering(commandBuffer, &renderingInfo);
}

void VulkanCommandList::endRendering(Swapchain &swapchain) {
  auto &vkSwapchain = static_cast<VulkanSwapchain &>(swapchain);
  uint32_t frameIndex = vkSwapchain.getCurrentFrameIndex();
  VkImage image = vkSwapchain.getImage(frameIndex);

  vkCmdEndRendering(commandBuffer);

  // transition for presentations
  VkImageMemoryBarrier2 imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  imageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  imageBarrier.dstAccessMask = 0;
  imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  imageBarrier.image = image;
  imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.levelCount = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &imageBarrier;

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}
void VulkanCommandList::endRendering(Texture *renderTarget) {
  auto *vk13Texture = static_cast<VulkanTexture *>(renderTarget);
  VkImage image = vk13Texture->getImage();

  vkCmdEndRendering(commandBuffer);

  // transition the texture so it is ready to be sampled by shaders in the
  // next pass
  VkImageMemoryBarrier2 imageBarrier{};
  imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  imageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
  imageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
  imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageBarrier.image = image;
  imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.levelCount = 1;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &imageBarrier;

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

void VulkanCommandList::setViewport(float x, float y, float width,
                                    float height) {
  VkViewport viewport{};
  viewport.x = x;
  viewport.y = height;
  viewport.width = width;
  viewport.height = -height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}
void VulkanCommandList::setScissor(int32_t x, int32_t y, uint32_t width,
                                   uint32_t height) {
  VkRect2D scissor{};
  scissor.offset = {x, y};
  scissor.extent = {width, height};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanCommandList::bindTexture(uint32_t bindingSlot, Texture *texture) {
  auto *vk13Texture = static_cast<VulkanTexture *>(texture);

  VkDescriptorImageInfo descImageInfo{};
  descImageInfo.imageLayout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // t register
  descImageInfo.imageView = vk13Texture->getImageView();

  VkWriteDescriptorSet writeDesc{};
  writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDesc.dstBinding = bindingSlot;
  writeDesc.descriptorCount = 1;
  writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  writeDesc.pImageInfo = &descImageInfo;

  auto pushFunc = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(
      device.getLogicalDevice(), "vkCmdPushDescriptorSetKHR");
  if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
    pushFunc(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
             computePiplineLayout, 0, 1, &writeDesc);
  } else {
    pushFunc(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
             graphicsPiplineLayout, 0, 1, &writeDesc);
  }
}

void VulkanCommandList::bindStorageImage(uint32_t bindingSlot,
                                         Texture *texture) {
  auto *vk13Texture = static_cast<VulkanTexture *>(texture);

  VkDescriptorImageInfo descImageInfo{};
  descImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // u registers
  descImageInfo.imageView = vk13Texture->getImageView();

  VkWriteDescriptorSet writeDesc{};
  writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDesc.dstBinding = bindingSlot;
  writeDesc.descriptorCount = 1;
  writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writeDesc.pImageInfo = &descImageInfo;

  auto pushFunc = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(
      device.getLogicalDevice(), "vkCmdPushDescriptorSetKHR");
  if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
    pushFunc(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
             computePiplineLayout, 0, 1, &writeDesc);
  } else {
    pushFunc(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
             graphicsPiplineLayout, 0, 1, &writeDesc);
  }
}

void VulkanCommandList::bindSampler(uint32_t bindingSlot) {
  VkDescriptorImageInfo samplerInfo{};
  samplerInfo.sampler = device.getLinearSampler();

  VkWriteDescriptorSet writeDesc{};
  writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDesc.dstBinding = bindingSlot;
  writeDesc.descriptorCount = 1;
  writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  writeDesc.pImageInfo = &samplerInfo;

  auto pushFunc = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(
      device.getLogicalDevice(), "vkCmdPushDescriptorSetKHR");
  if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
    pushFunc(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
             computePiplineLayout, 0, 1, &writeDesc);
  } else {
    pushFunc(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
             graphicsPiplineLayout, 0, 1, &writeDesc);
  }
}

void VulkanCommandList::transitionTexture(Texture *texture, ResourceState from,
                                          ResourceState to) {
  auto *vk13Texture = static_cast<VulkanTexture *>(texture);

  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.image = vk13Texture->getImage();
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  // Initializing memory
  if (from == ResourceState::Undefined &&
      to == ResourceState::UnorderedAccess) {
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    barrier.srcAccessMask = 0;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  }
  //  Read to Write
  else if (from == ResourceState::ShaderResource &&
           to == ResourceState::UnorderedAccess) {
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                           VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  }
  // Write to Read
  else if (from == ResourceState::UnorderedAccess &&
           to == ResourceState::ShaderResource) {
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                           VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
  }

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &barrier;

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

void VulkanCommandList::transitionBuffer(Buffer *buffer, ResourceState from,
                                         ResourceState to) {
  auto *vk13buffer = static_cast<VulkanBuffer *>(buffer);

  VkBufferMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = vk13buffer->getVkBuffer();
  barrier.offset = 0;
  barrier.size = VK_WHOLE_SIZE;

  if (from == ResourceState::UnorderedAccess &&
      to == ResourceState::ShaderResource) {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                           VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
  } else if (from == ResourceState::ShaderResource &&
             to == ResourceState::UnorderedAccess) {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                           VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    // compute to compute barrier
  } else if (from == ResourceState::UnorderedAccess &&
             to == ResourceState::UnorderedAccess) {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    // We need to wait for writes to finish before we Read OR Write again
    barrier.dstAccessMask =
        VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  } else if (from == ResourceState::TransferDst &&
             to == ResourceState::UnorderedAccess) {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.dstAccessMask =
        VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  } else if (from == ResourceState::UnorderedAccess &&
             to == ResourceState::TransferDst) {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.srcAccessMask =
        VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT;
    barrier.dstStageMask =
        VK_PIPELINE_STAGE_2_CLEAR_BIT; // vkCmdFillBuffer stage
    barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  }

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.bufferMemoryBarrierCount = 1;
  depInfo.pBufferMemoryBarriers = &barrier;

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

void VulkanCommandList::bindPipeline(Pipeline &pipeline) {
  currentPipeline = &pipeline;
  if (pipeline.getBindPoint() == PipelineBindPoint::Graphics) {
    auto &vk13Pipeline = static_cast<VulkanPipeline &>(pipeline);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vk13Pipeline.getNativePipeline());
    graphicsPiplineLayout = vk13Pipeline.getNativeLayout();
  } else { // compute
    auto &vk13Pipeline = static_cast<VulkanComputePipeline &>(pipeline);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      vk13Pipeline.getNativePipeline());
    computePiplineLayout = vk13Pipeline.getNativeLayout();
  }
}

void VulkanCommandList::dispatch(uint32_t groupCountX, uint32_t groupCountY,
                                 uint32_t groupCountZ) {
  vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandList::draw(uint32_t vertexCount, uint32_t instanceCount,
                             uint32_t firstVertex, uint32_t firstInstance) {
  vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex,
            firstInstance);
}

void VulkanCommandList::bindVertexBuffer(Buffer *buffer, size_t stride) {
  auto *vk13Buffer = static_cast<VulkanBuffer *>(buffer);

  VkBuffer buffers[] = {vk13Buffer->getVkBuffer()};
  VkDeviceSize offsets[] = {0};

  // Bind vertex stream to layout slot 0
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void VulkanCommandList::bindStorageBuffer(uint32_t bindingSlot,
                                          Buffer *buffer) {
  auto *vkBuffer = static_cast<VulkanBuffer *>(buffer);

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = vkBuffer->getVkBuffer();
  bufferInfo.offset = 0;
  bufferInfo.range = VK_WHOLE_SIZE;

  VkWriteDescriptorSet writeDescSet{};
  writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescSet.dstBinding = bindingSlot;
  writeDescSet.descriptorCount = 1;
  writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writeDescSet.pBufferInfo = &bufferInfo;

  auto pushFunc = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(
      device.getLogicalDevice(), "vkCmdPushDescriptorSetKHR");
  if (pushFunc) {
    if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
      pushFunc(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
               computePiplineLayout, 0, 1, &writeDescSet);
    } else { // graphics
      pushFunc(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
               graphicsPiplineLayout, 0, 1, &writeDescSet);
    }
  }
}

void VulkanCommandList::pushConstants(uint32_t offset, uint32_t size,
                                      const void *data, ShaderStage stage) {
  VkShaderStageFlags vkStage = mapShaderStage(stage);

  if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
    vkCmdPushConstants(commandBuffer, computePiplineLayout, vkStage, offset,
                       size, data);
  } else {
    vkCmdPushConstants(commandBuffer, graphicsPiplineLayout, vkStage, offset,
                       size, data);
  }
}

void VulkanCommandList::clearBuffer(Buffer *buffer, uint32_t value) {
  auto *vkBuffer = static_cast<VulkanBuffer *>(buffer);
  vkCmdFillBuffer(commandBuffer, vkBuffer->getVkBuffer(), 0, VK_WHOLE_SIZE,
                  value);
}
} // namespace elementalEngine::RHI