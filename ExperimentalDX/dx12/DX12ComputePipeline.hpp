#pragma once

#include "DX12Device.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include <unordered_map>
namespace elementalEngine::RHI {
class DX12ComputePipeline : public Pipeline {
public:
  DX12ComputePipeline(DX12Device &device, const std::string &shaderName,
                      const PipelineConfig &config);
  ~DX12ComputePipeline() override;

  PipelineBindPoint getBindPoint() const override {
    return PipelineBindPoint::Compute;
  }
  ID3D12PipelineState *getNativePipelineState() const {
    return pipelineState.Get();
  }
  ID3D12RootSignature *getNativeRootSignature() const {
    return rootSignature.Get();
  }
  UINT getPushConstantRootIndex() const { return pushConstantRootIndex; }

  // bridging the RHI binding slot to the DX12 Root Parameter Index
  UINT getRootIndex(uint32_t bindingSlot) const {
    auto it = slotToRootIndex.find(bindingSlot);
    return (it != slotToRootIndex.end()) ? it->second : ~0u;
  }

private:
  DX12Device &device;
  ComPtr<ID3D12RootSignature> rootSignature;
  ComPtr<ID3D12PipelineState> pipelineState;
  std::unordered_map<uint32_t, UINT> slotToRootIndex;
  UINT pushConstantRootIndex{~0u};

  void createPipeline(const std::string &shaderName,
                      const PipelineConfig &config);
};

} // namespace elementalEngine::RHI