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

private:
  VulkanDevice &device;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkShaderModule createShaderModule(const std::vector<char> &code);
  void createPipeline(VkFormat colorAttachmentFormat);
};
} // namespace elementalEngine::RHI