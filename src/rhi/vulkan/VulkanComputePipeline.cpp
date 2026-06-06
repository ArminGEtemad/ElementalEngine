#include "VulkanComputePipeline.hpp"
#include "FileHandling.hpp"
#include "VulkanDevice.hpp"

#include <cstdint>
#include <stdexcept>

namespace elementalEngine::RHI {
VulkanComputePipeline::VulkanComputePipeline(VulkanDevice &device)
    : device(device) {
  createPipeline();
}

VulkanComputePipeline::~VulkanComputePipeline() {
  vkDestroyPipeline(device.getLogicalDevice(), computePipeline, nullptr);
  vkDestroyPipelineLayout(device.getLogicalDevice(), pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(device.getLogicalDevice(), descriptorSetLayout,
                               nullptr);
}

VkShaderModule
VulkanComputePipeline::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo shaderCreateInfo{};
  shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderCreateInfo.codeSize = code.size();
  shaderCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device.getLogicalDevice(), &shaderCreateInfo,
                           nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Compute Shader Module!");
  }
  return shaderModule;
}

void VulkanComputePipeline::createPipeline() {
  auto compShaderCode = Core::readFile("build/compute.spv");
  VkShaderModule compShaderModule = createShaderModule(compShaderCode);

  VkPipelineShaderStageCreateInfo compShaderStageInfo{};
  compShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  compShaderStageInfo.module = compShaderModule;
  compShaderStageInfo.pName = "CSMain";

  VkDescriptorSetLayoutBinding bindingSetLayout{};
  bindingSetLayout.binding = 0;
  bindingSetLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindingSetLayout.descriptorCount = 1;
  bindingSetLayout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
  setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  setLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
  setLayoutInfo.bindingCount = 1;
  setLayoutInfo.pBindings = &bindingSetLayout;

  if (vkCreateDescriptorSetLayout(device.getLogicalDevice(), &setLayoutInfo,
                                  nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create compute descriptor layout!");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

  if (vkCreatePipelineLayout(device.getLogicalDevice(), &pipelineLayoutInfo,
                             nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create compute pipeline layout!");
  }

  VkComputePipelineCreateInfo computePipelineInfo{};
  computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineInfo.layout = pipelineLayout;
  computePipelineInfo.stage = compShaderStageInfo;

  if (vkCreateComputePipelines(device.getLogicalDevice(), VK_NULL_HANDLE, 1,
                               &computePipelineInfo, nullptr,
                               &computePipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create compute pipeline!");
  }

  // destroy after calculation
  vkDestroyShaderModule(device.getLogicalDevice(), compShaderModule, nullptr);
}

} // namespace elementalEngine::RHI