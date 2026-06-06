#pragma once

#include "ComputePipeline.hpp"
#include "DX12Device.hpp"

namespace elementalEngine::RHI {
class DX12ComputePipeline : public ComputePipeline {
public:
  DX12ComputePipeline(DX12Device &device);
  ~DX12ComputePipeline() override;

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

  void createPipeline();
};

} // namespace elementalEngine::RHI