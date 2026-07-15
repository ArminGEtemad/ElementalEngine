#include "VulkanComputePipeline.hpp"
#include "FileHandling.hpp"
#include "VulkanDevice.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace elementalEngine::RHI {

// helper function
// TODO refactor since we have to duplicate it for the graphics pipeline too
static VkDescriptorType mapDescriptorType(DescriptorType type) {
  switch (type) {
  case DescriptorType::Sampler:
    return VK_DESCRIPTOR_TYPE_SAMPLER;
  case DescriptorType::SampledImage:
    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  case DescriptorType::StorageImage:
    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  case DescriptorType::UniformBuffer:
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  case DescriptorType::StorageBuffer:
    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  default:
    throw std::runtime_error("Unsupported Descriptor Type!");
  }
}

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

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice &device,
                                             const std::string &shaderName,
                                             const PipelineConfig &config)
    : device(device) {
  createPipeline(shaderName, config);
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

void VulkanComputePipeline::createPipeline(const std::string &shaderName,
                                           const PipelineConfig &config) {
  // read the compiled shaders
  std::string filepath = "build/" + shaderName + ".spv";
  auto compShaderCode = Core::readFile(filepath);
  VkShaderModule compShaderModule = createShaderModule(compShaderCode);

  VkPipelineShaderStageCreateInfo compShaderStageInfo{};
  compShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  compShaderStageInfo.module = compShaderModule;
  compShaderStageInfo.pName = "CSMain";

  // dynamical bindings for different pyhsical dynamics
  std::vector<VkDescriptorSetLayoutBinding> vkBindings;
  for (const auto &binding : config.bindings) {
    VkDescriptorSetLayoutBinding bindingsLayout{};
    bindingsLayout.binding = binding.bindingSlot;
    bindingsLayout.descriptorType = mapDescriptorType(binding.type);
    bindingsLayout.descriptorCount = binding.count;
    bindingsLayout.stageFlags = mapShaderStage(binding.stage);
    vkBindings.push_back(bindingsLayout);
  }

  VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
  setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  setLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
  setLayoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
  setLayoutInfo.pBindings = vkBindings.data();

  if (vkCreateDescriptorSetLayout(device.getLogicalDevice(), &setLayoutInfo,
                                  nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create compute descriptor layout!");
  }

  // dynamical push constant buffers
  std::vector<VkPushConstantRange> pushRanges;
  if (config.pushConstants.size > 0) {
    VkPushConstantRange range{};
    range.stageFlags = mapShaderStage(config.pushConstants.stage);
    range.offset = config.pushConstants.offset;
    range.size = config.pushConstants.size;
    pushRanges.push_back(range);
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount =
      static_cast<uint32_t>(pushRanges.size());
  pipelineLayoutInfo.pPushConstantRanges = pushRanges.data();

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