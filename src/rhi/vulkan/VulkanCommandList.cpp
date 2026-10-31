#include "VulkanCommandList.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include "Texture.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanComputePipeline.hpp"
#include "VulkanDevice.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanTexture.hpp"
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace elementalEngine::RHI {

static PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT = nullptr;
static PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT = nullptr;
static PFN_vkCmdInsertDebugUtilsLabelEXT pfnCmdInsertDebugUtilsLabelEXT =
    nullptr;
static PFN_vkCmdPushDescriptorSetKHR pfnCmdPushDescriptorSetKHR = nullptr;

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

struct BufferStateInfo {
  VkPipelineStageFlags2 stageMask;
  VkAccessFlags2 accessMask;
};

static BufferStateInfo getBufferStateInfo(ResourceState state) {
  switch (state) {
  case ResourceState::Undefined:
    return {VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0};
  case ResourceState::UnorderedAccess:
    return {VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT};
  case ResourceState::ShaderResource:
    return {VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT};
  case ResourceState::TransferDst:
    return {VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT};
  case ResourceState::TransferSrc:
    return {VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT};
  case ResourceState::IndirectArgument:
    return {VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT};
  default:
    throw std::invalid_argument("Unsupported Buffer ResourceState transition!");
  }
}

struct TextureStateInfo {
  VkPipelineStageFlags2 stageMask;
  VkAccessFlags2 accessMask;
  VkImageLayout layout;
};

static TextureStateInfo getTextureStateInfo(ResourceState state) {
  switch (state) {
  case ResourceState::Undefined:
    return {VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED};
  case ResourceState::UnorderedAccess:
    return {VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL};
  case ResourceState::ShaderResource:
    return {VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  case ResourceState::RenderTarget:
    return {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  case ResourceState::DepthStencilWrite:
    return {VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
  case ResourceState::DepthStencilReadOnly:
    return {VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL};
  case ResourceState::TransferDst:
    return {VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};
  case ResourceState::TransferSrc:
    return {VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
  case ResourceState::Present:
    return {VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
  default:
    throw std::invalid_argument(
        "Unsupported Texture ResourceState transition!");
  }
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

  if (!pfnCmdBeginDebugUtilsLabelEXT) {
    VkInstance instance = device.getInstance();
    pfnCmdBeginDebugUtilsLabelEXT =
        (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance, "vkCmdBeginDebugUtilsLabelEXT");
    pfnCmdEndDebugUtilsLabelEXT =
        (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance, "vkCmdEndDebugUtilsLabelEXT");
    pfnCmdInsertDebugUtilsLabelEXT =
        (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(
            instance, "vkCmdInsertDebugUtilsLabelEXT");
  }

  if (!pfnCmdPushDescriptorSetKHR) {
    pfnCmdPushDescriptorSetKHR =
        (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(
            device.getLogicalDevice(), "vkCmdPushDescriptorSetKHR");
  }

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

void VulkanCommandList::beginRendering(const RenderingInfo &info) {
  std::vector<VkRenderingAttachmentInfo> colorAttachments;
  colorAttachments.reserve(info.colorAttachments.size());

  // transition and setting up the color attachment
  for (const auto &att : info.colorAttachments) {
    auto *vkTex = static_cast<VulkanTexture *>(att.texture);

    VkRenderingAttachmentInfo vkAtt{};
    vkAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    vkAtt.imageView = vkTex->getImageView();
    vkAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    vkAtt.loadOp =
        att.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    vkAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    vkAtt.clearValue.color = {{att.clearColor[0], att.clearColor[1],
                               att.clearColor[2], att.clearColor[3]}};
    colorAttachments.push_back(vkAtt);
  }

  // transition and setting up depth attachment if present
  VkRenderingAttachmentInfo depthAttachmentInfo{};
  bool hasDepth = info.depthAttachment.has_value();

  if (hasDepth) {
    const auto &depthAtt = info.depthAttachment.value();
    auto *vkDepthTex = static_cast<VulkanTexture *>(depthAtt.texture);

    depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachmentInfo.imageView = vkDepthTex->getImageView();
    depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachmentInfo.loadOp = depthAtt.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                : VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachmentInfo.clearValue.depthStencil = {depthAtt.clearDepth,
                                                   depthAtt.clearStencil};
  }

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea = {{0, 0}, {info.renderWidth, info.renderHeight}};
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount =
      static_cast<uint32_t>(colorAttachments.size());
  renderingInfo.pColorAttachments = colorAttachments.data();
  renderingInfo.pDepthAttachment = hasDepth ? &depthAttachmentInfo : nullptr;

  vkCmdBeginRendering(commandBuffer, &renderingInfo);
}

void VulkanCommandList::endRendering() { vkCmdEndRendering(commandBuffer); }

void VulkanCommandList::setViewport(float x, float y, float width,
                                    float height) {
  VkViewport viewport{};
  viewport.x = x;
  viewport.y = y + height;
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

  if (pfnCmdPushDescriptorSetKHR) {
    if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
      pfnCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                 computePiplineLayout, 0, 1, &writeDesc);
    } else {
      pfnCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 graphicsPiplineLayout, 0, 1, &writeDesc);
    }
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

  if (pfnCmdPushDescriptorSetKHR) {
    if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
      pfnCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                 computePiplineLayout, 0, 1, &writeDesc);
    } else {
      pfnCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 graphicsPiplineLayout, 0, 1, &writeDesc);
    }
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

  if (pfnCmdPushDescriptorSetKHR) {
    if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
      pfnCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                 computePiplineLayout, 0, 1, &writeDesc);
    } else {
      pfnCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 graphicsPiplineLayout, 0, 1, &writeDesc);
    }
  }
}

void VulkanCommandList::bindIndexBuffer(Buffer *buffer, IndexType indexType,
                                        size_t offset) {
  auto *vkBuffer = static_cast<VulkanBuffer *>(buffer);
  VkIndexType vkIndexType = (indexType == IndexType::Uint16)
                                ? VK_INDEX_TYPE_UINT16
                                : VK_INDEX_TYPE_UINT32;
  vkCmdBindIndexBuffer(commandBuffer, vkBuffer->getVkBuffer(), offset,
                       vkIndexType);
}

void VulkanCommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                    uint32_t firstIndex, int32_t vertexOffset,
                                    uint32_t firstInstance) {
  vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex,
                   vertexOffset, firstInstance);
}

void VulkanCommandList::transitionTexture(Texture *texture, ResourceState to) {
  ResourceState from = texture->getState();
  if (from == to && to != ResourceState::UnorderedAccess) {
    return;
  }

  auto *vk13Texture = static_cast<VulkanTexture *>(texture);
  TextureFormat fmt = vk13Texture->getFormat();

  bool isDepth = (fmt == TextureFormat::D32_FLOAT ||
                  fmt == TextureFormat::D24_UNORM_S8_UINT);

  VkImageAspectFlags aspectFlags =
      isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  if (fmt == TextureFormat::D24_UNORM_S8_UINT) {
    aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  TextureStateInfo srcInfo = getTextureStateInfo(from);
  TextureStateInfo dstInfo = getTextureStateInfo(to);

  if (from == ResourceState::Undefined && to == ResourceState::RenderTarget) {
    srcInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  }

  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.srcStageMask = srcInfo.stageMask;
  barrier.srcAccessMask = srcInfo.accessMask;
  barrier.dstStageMask = dstInfo.stageMask;
  barrier.dstAccessMask = dstInfo.accessMask;
  barrier.oldLayout = srcInfo.layout;
  barrier.newLayout = dstInfo.layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = vk13Texture->getImage();
  barrier.subresourceRange.aspectMask = aspectFlags;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &barrier;

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);
  texture->setState(to);
}

void VulkanCommandList::transitionBuffer(Buffer *buffer, ResourceState to) {
  ResourceState from = buffer->getState();
  if (from == to && to != ResourceState::UnorderedAccess) {
    return;
  }

  auto *vk13buffer = static_cast<VulkanBuffer *>(buffer);
  BufferStateInfo srcInfo = getBufferStateInfo(from);
  BufferStateInfo dstInfo = getBufferStateInfo(to);

  VkBufferMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barrier.srcStageMask = srcInfo.stageMask;
  barrier.srcAccessMask = srcInfo.accessMask;
  barrier.dstStageMask = dstInfo.stageMask;
  barrier.dstAccessMask = dstInfo.accessMask;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = vk13buffer->getVkBuffer();
  barrier.offset = 0;
  barrier.size = VK_WHOLE_SIZE;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.bufferMemoryBarrierCount = 1;
  depInfo.pBufferMemoryBarriers = &barrier;

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);
  buffer->setState(to);
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

  if (pfnCmdPushDescriptorSetKHR) {
    if (currentPipeline->getBindPoint() == PipelineBindPoint::Compute) {
      pfnCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                 computePiplineLayout, 0, 1, &writeDescSet);
    } else { // graphics
      pfnCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 graphicsPiplineLayout, 0, 1, &writeDescSet);
    }
  }
}

void VulkanCommandList::bindUniformBuffer(uint32_t bindingSlot, Buffer *buffer,
                                          size_t offset, size_t range) {

  auto *vkBuffer = static_cast<VulkanBuffer *>(buffer);

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = vkBuffer->getVkBuffer();
  bufferInfo.offset = offset;
  bufferInfo.range = (range == 0) ? VK_WHOLE_SIZE : range;

  VkWriteDescriptorSet writeDescSet{};
  writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescSet.dstBinding = bindingSlot;
  writeDescSet.descriptorCount = 1;
  writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeDescSet.pBufferInfo = &bufferInfo;

  if (pfnCmdPushDescriptorSetKHR) {
    VkPipelineBindPoint bindPoint =
        (currentPipeline->getBindPoint() == PipelineBindPoint::Compute)
            ? VK_PIPELINE_BIND_POINT_COMPUTE
            : VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkPipelineLayout layout = (bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE)
                                  ? computePiplineLayout
                                  : graphicsPiplineLayout;
    pfnCmdPushDescriptorSetKHR(commandBuffer, bindPoint, layout, 0, 1,
                               &writeDescSet);
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

void VulkanCommandList::copyBuffer(Buffer *src, Buffer *dst, size_t size,
                                   size_t srcOffset, size_t dstOffset) {
  transitionBuffer(src, ResourceState::TransferSrc);
  transitionBuffer(dst, ResourceState::TransferDst);

  auto *vkSrc = static_cast<VulkanBuffer *>(src);
  auto *vkDst = static_cast<VulkanBuffer *>(dst);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = srcOffset;
  copyRegion.dstOffset = dstOffset;
  copyRegion.size = size;

  vkCmdCopyBuffer(commandBuffer, vkSrc->getVkBuffer(), vkDst->getVkBuffer(), 1,
                  &copyRegion);
}

void VulkanCommandList::drawIndirect(Buffer *buffer, size_t offset,
                                     uint32_t drawCount, uint32_t stride) {
  auto *vkBuffer = static_cast<VulkanBuffer *>(buffer);
  vkCmdDrawIndirect(commandBuffer, vkBuffer->getVkBuffer(), offset, drawCount,
                    stride);
};

void VulkanCommandList::drawIndexedIndirect(Buffer *buffer, size_t offset,
                                            uint32_t drawCount,
                                            uint32_t stride) {
  auto *vkBuffer = static_cast<VulkanBuffer *>(buffer);
  vkCmdDrawIndexedIndirect(commandBuffer, vkBuffer->getVkBuffer(), offset,
                           drawCount, stride);
}

void VulkanCommandList::beginDebugMarker(const char *name,
                                         const float color[4]) {
  if (!pfnCmdBeginDebugUtilsLabelEXT)
    return;

  VkDebugUtilsLabelEXT labelInfo{};
  labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  labelInfo.pLabelName = name;
  if (color) {
    labelInfo.color[0] = color[0];
    labelInfo.color[1] = color[1];
    labelInfo.color[2] = color[2];
    labelInfo.color[3] = color[3];
  } else {
    // Default white
    labelInfo.color[0] = 1.0f;
    labelInfo.color[1] = 1.0f;
    labelInfo.color[2] = 1.0f;
    labelInfo.color[3] = 1.0f;
  }

  pfnCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
}

void VulkanCommandList::endDebugMarker() {
  if (!pfnCmdEndDebugUtilsLabelEXT)
    return;
  pfnCmdEndDebugUtilsLabelEXT(commandBuffer);
}

void VulkanCommandList::insertDebugMarker(const char *name,
                                          const float color[4]) {
  if (!pfnCmdInsertDebugUtilsLabelEXT)
    return;

  VkDebugUtilsLabelEXT labelInfo{};
  labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  labelInfo.pLabelName = name;
  if (color) {
    labelInfo.color[0] = color[0];
    labelInfo.color[1] = color[1];
    labelInfo.color[2] = color[2];
    labelInfo.color[3] = color[3];
  } else {
    labelInfo.color[0] = 1.0f;
    labelInfo.color[1] = 1.0f;
    labelInfo.color[2] = 1.0f;
    labelInfo.color[3] = 1.0f;
  }

  pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &labelInfo);
}
} // namespace elementalEngine::RHI
