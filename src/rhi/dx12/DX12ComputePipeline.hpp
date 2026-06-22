#pragma once

#include "DX12Device.hpp"
#include "RHICommon.hpp"
namespace elementalEngine::RHI {
class DX12ComputePipeline : public Pipeline {
public:
  DX12ComputePipeline(DX12Device &device, const std::string &shaderName);
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

private:
  DX12Device &device;
  ComPtr<ID3D12RootSignature> rootSignature;
  ComPtr<ID3D12PipelineState> pipelineState;

  void createPipeline(const std::string &shaderName);
};

} // namespace elementalEngine::RHI