#pragma once
#include "DX12Device.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"

namespace elementalEngine::RHI {
class DX12Pipeline : public Pipeline {
public:
  DX12Pipeline(DX12Device &device, const std::string &vertexShaderName,
               const std::string &fragmentShaderName);
  ~DX12Pipeline() override;

  PipelineBindPoint getBindPoint() const override {
    return PipelineBindPoint::Graphics;
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

  void createPipeline(const std::string &vertexShaderName,
                      const std::string &fragmentShaderName);
};
} // namespace elementalEngine::RHI