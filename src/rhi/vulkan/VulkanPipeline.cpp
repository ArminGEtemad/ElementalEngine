#include "VulkanPipeline.hpp"
#include "FileHandling.hpp"
#include "VulkanDevice.hpp"

#include <stdexcept>

namespace elementalEngine::RHI {
VulkanPipeline::VulkanPipeline(VulkanDevice &device,
                               VkFormat colorAttachmentFormat,
                               const std::string &vertexShaderName,
                               const std::string &fragmentShaderName)
    : device(device) {
  createPipeline(colorAttachmentFormat, vertexShaderName, fragmentShaderName);
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
                                    const std::string &fragmentShaderName) {
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

  VkDescriptorSetLayoutBinding bindingSetLayout;

  // vertex has no binding
  // fragment bindings
  // read density
  bindingSetLayout.binding = 1;
  bindingSetLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  bindingSetLayout.descriptorCount = 1;
  bindingSetLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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

  // uniform buffer
  VkPushConstantRange pushConstRange{};
  pushConstRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstRange.offset = 0;
  pushConstRange.size = sizeof(SimConfig);

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
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

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
  colorBlendAttachment.blendEnable = VK_FALSE;

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
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;

  if (vkCreatePipelineLayout(device.getLogicalDevice(), &pipelineLayoutInfo,
                             nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout!");
  }

  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
  pipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  pipelineRenderingCreateInfo.colorAttachmentCount = 1;
  pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;

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