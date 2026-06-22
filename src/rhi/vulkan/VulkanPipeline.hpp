#pragma once

#include "Pipeline.hpp"
#include "VulkanDevice.hpp"
#include <vector>

namespace elementalEngine::RHI {
class VulkanPipeline : public Pipeline {
public:
  VulkanPipeline(VulkanDevice &device, VkFormat colorAttachmentFormat,
                 const std::string &vertexShaderName,
                 const std::string &fragmentShaderName);
  ~VulkanPipeline() override;

  PipelineBindPoint getBindPoint() const override {
    return PipelineBindPoint::Graphics;
  }
  VkPipeline getNativePipeline() const { return graphicsPipeline; }
  VkPipelineLayout getNativeLayout() const { return pipelineLayout; }

private:
  VulkanDevice &device;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};

  VkShaderModule createShaderModule(const std::vector<char> &code);
  void createPipeline(VkFormat colorAttachmentFormat,
                      const std::string &vertexShaderName,
                      const std::string &fragmentShaderName);
};
} // namespace elementalEngine::RHI