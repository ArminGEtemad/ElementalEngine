#pragma once

#include "Pipeline.hpp"
#include "VulkanDevice.hpp"
#include <vector>

namespace elementalEngine::RHI {
class VulkanPipeline : public Pipeline {
public:
  VulkanPipeline(VulkanDevice &device, VkFormat colorAttachmentFormat);
  ~VulkanPipeline() override;

  VkPipeline getNativePipeline() const { return graphicsPipeline; }
  VkPipelineLayout getNativeLayout() const { return pipelineLayout; }

private:
  VulkanDevice &device;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};

  VkShaderModule createShaderModule(const std::vector<char> &code);
  void createPipeline(VkFormat colorAttachmentFormat);
};
} // namespace elementalEngine::RHI