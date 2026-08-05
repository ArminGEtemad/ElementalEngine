#include "VulkanPipeline.hpp"
#include "FileHandling.hpp"
#include "Pipeline.hpp"
#include "VulkanDevice.hpp"
#include "VulkanTexture.hpp"

#include <cstdint>
#include <stdexcept>

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

static VkCompareOp mapCompare(CompareOp op) {
  switch (op) {
  case CompareOp::Never:
    return VK_COMPARE_OP_NEVER;
  case CompareOp::Less:
    return VK_COMPARE_OP_LESS;
  case CompareOp::Equal:
    return VK_COMPARE_OP_EQUAL;
  case CompareOp::LessOrEqual:
    return VK_COMPARE_OP_LESS_OR_EQUAL;
  case CompareOp::Greater:
    return VK_COMPARE_OP_GREATER;
  case CompareOp::GreaterOrEqual:
    return VK_COMPARE_OP_GREATER_OR_EQUAL;
  case CompareOp::NotEqual:
    return VK_COMPARE_OP_NOT_EQUAL;
  case CompareOp::Always:
    return VK_COMPARE_OP_ALWAYS;
  }
  return VK_COMPARE_OP_LESS;
}

static VkCullModeFlags mapCullMode(CullMode mode) {
  switch (mode) {
  case CullMode::None:
    return VK_CULL_MODE_NONE;
  case CullMode::Front:
    return VK_CULL_MODE_FRONT_BIT;
  case CullMode::Back:
    return VK_CULL_MODE_BACK_BIT;
  }
  return VK_CULL_MODE_BACK_BIT;
}

VulkanPipeline::VulkanPipeline(VulkanDevice &device,
                               VkFormat colorAttachmentFormat,
                               const std::string &vertexShaderName,
                               const std::string &fragmentShaderName,
                               const PipelineConfig &config)
    : device(device) {

  createPipeline(colorAttachmentFormat, vertexShaderName, fragmentShaderName,
                 config);
};
VulkanPipeline::~VulkanPipeline() {
  vkDestroyPipeline(device.getLogicalDevice(), graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device.getLogicalDevice(), pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(device.getLogicalDevice(), descriptorSetLayout,
                               nullptr);
};

// creating the shader modul
VkShaderModule
VulkanPipeline::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo shaderCreateInfo{};
  shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderCreateInfo.codeSize = code.size();
  shaderCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device.getLogicalDevice(), &shaderCreateInfo,
                           nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader modules!");
  }
  return shaderModule;
}

void VulkanPipeline::createPipeline(VkFormat colorAttachmentFormat,
                                    const std::string &vertexShaderName,
                                    const std::string &fragmentShaderName,
                                    const PipelineConfig &config) {
  // make shder code -----------
  std::string vertFilepath = "build/" + vertexShaderName + ".spv";
  std::string fragFilepath = "build/" + fragmentShaderName + ".spv";
  auto vertShaderCode = Core::readFile(vertFilepath);
  auto fragShaderCode = Core::readFile(fragFilepath);

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
  // ----------------------------

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "VSMain";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "FSMain";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

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

  std::vector<VkPushConstantRange> pushRanges;
  if (config.pushConstants.size > 0) {
    VkPushConstantRange range{};
    range.stageFlags = mapShaderStage(config.pushConstants.stage);
    range.offset = config.pushConstants.offset;
    range.size = config.pushConstants.size;
    pushRanges.push_back(range);
  }

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;

  // Input Assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Dynamic Viewport and Scissor
  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = mapCullMode(config.cullMode);
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  // depth stencil
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable =
      config.depthState.depthTestEnable ? VK_TRUE : VK_FALSE;
  depthStencil.depthWriteEnable =
      config.depthState.depthWriteEnable ? VK_TRUE : VK_FALSE;
  depthStencil.depthCompareOp = mapCompare(config.depthState.depthCompareOp);
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkFormat vkDepthFormat = VulkanTexture::mapFormat(config.depthFomat);

  // Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // Color Blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  if (config.blendMode == Blendmode::Additive) {
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  } else {
    colorBlendAttachment.blendEnable = VK_FALSE;
  }
  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  // Pipeline Layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount =
      static_cast<uint32_t>(pushRanges.size());
  pipelineLayoutInfo.pPushConstantRanges = pushRanges.data();

  if (vkCreatePipelineLayout(device.getLogicalDevice(), &pipelineLayoutInfo,
                             nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout!");
  }

  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
  pipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  pipelineRenderingCreateInfo.colorAttachmentCount = 1;
  pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;
  pipelineRenderingCreateInfo.depthAttachmentFormat =
      config.hasDepthAttachment ? vkDepthFormat : VK_FORMAT_UNDEFINED;

  // Final Pipeline Creation
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = &pipelineRenderingCreateInfo;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = VK_NULL_HANDLE;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device.getLogicalDevice(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline!");
  }

  // Cleanup
  vkDestroyShaderModule(device.getLogicalDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(device.getLogicalDevice(), vertShaderModule, nullptr);
}

} // namespace elementalEngine::RHI