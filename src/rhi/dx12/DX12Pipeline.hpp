#pragma once
#include "DX12Device.hpp"
#include "Pipeline.hpp"

namespace elementalEngine::RHI {
class DX12Pipeline : public Pipeline {
public:
  DX12Pipeline(DX12Device &device);
  ~DX12Pipeline() override;

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