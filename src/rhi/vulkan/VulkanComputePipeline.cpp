#include "VulkanComputePipeline.hpp"
#include "FileHandling.hpp"
#include "VulkanDevice.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace elementalEngine::RHI {
VulkanComputePipeline::VulkanComputePipeline(VulkanDevice &device,
                                             const std::string &shaderName)
    : device(device) {
  createPipeline(shaderName);
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

void VulkanComputePipeline::createPipeline(const std::string &shaderName) {
  std::string filepath = "build/" + shaderName + ".spv";
  auto compShaderCode = Core::readFile(filepath);
  VkShaderModule compShaderModule = createShaderModule(compShaderCode);

  VkPipelineShaderStageCreateInfo compShaderStageInfo{};
  compShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  compShaderStageInfo.module = compShaderModule;
  compShaderStageInfo.pName = "CSMain";

  std::vector<VkDescriptorSetLayoutBinding> bindingSetLayout(4);

  // read density
  bindingSetLayout[0].binding = 1;
  bindingSetLayout[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindingSetLayout[0].descriptorCount = 1;
  bindingSetLayout[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // read velocity
  bindingSetLayout[1].binding = 2;
  bindingSetLayout[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindingSetLayout[1].descriptorCount = 1;
  bindingSetLayout[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // read and write density
  bindingSetLayout[2].binding = 3;
  bindingSetLayout[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindingSetLayout[2].descriptorCount = 1;
  bindingSetLayout[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  // read and write divergence
  bindingSetLayout[3].binding = 4;
  bindingSetLayout[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindingSetLayout[3].descriptorCount = 1;
  bindingSetLayout[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
  setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  setLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
  setLayoutInfo.bindingCount = static_cast<uint32_t>(bindingSetLayout.size());
  setLayoutInfo.pBindings = bindingSetLayout.data();

  if (vkCreateDescriptorSetLayout(device.getLogicalDevice(), &setLayoutInfo,
                                  nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create compute descriptor layout!");
  }

  VkPushConstantRange pushConstRange{};
  pushConstRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstRange.offset = 0;
  pushConstRange.size = sizeof(SimConfig);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;

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