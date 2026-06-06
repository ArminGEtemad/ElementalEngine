#pragma once

#include "ComputePipeline.hpp"
#include "VulkanDevice.hpp"
#include <vector>

namespace elementalEngine::RHI {
class VulkanComputePipeline : public ComputePipeline {
public:
  VulkanComputePipeline(VulkanDevice &device);
  ~VulkanComputePipeline() override;

  VkPipeline getNativePipeline() const { return computePipeline; }
  VkPipelineLayout getNativeLayout() const { return pipelineLayout; }

private:
  VulkanDevice &device;
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  VkPipeline computePipeline{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};

  VkShaderModule createShaderModule(const std::vector<char> &code);
  void createPipeline();
};

} // namespace elementalEngine::RHI