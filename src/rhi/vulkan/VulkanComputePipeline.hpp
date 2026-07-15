#pragma once

#include "Pipeline.hpp"
#include "VulkanDevice.hpp"
#include <vector>
namespace elementalEngine::RHI {
class VulkanComputePipeline : public Pipeline {
public:
  VulkanComputePipeline(VulkanDevice &device, const std::string &shaderName,
                        const PipelineConfig &config);
  ~VulkanComputePipeline() override;

  PipelineBindPoint getBindPoint() const override {
    return PipelineBindPoint::Compute;
  }
  VkPipeline getNativePipeline() const { return computePipeline; }
  VkPipelineLayout getNativeLayout() const { return pipelineLayout; }

private:
  VulkanDevice &device;
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  VkPipeline computePipeline{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};

  VkShaderModule createShaderModule(const std::vector<char> &code);
  void createPipeline(const std::string &shaderName,
                      const PipelineConfig &config);
};

} // namespace elementalEngine::RHI