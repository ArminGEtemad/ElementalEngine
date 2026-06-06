#include "DX12ComputePipeline.hpp"
#include "DX12Device.hpp"
#include "FileHandling.hpp"
#include <combaseapi.h>
#include <directx/d3d12.h>
#include <intsafe.h>
#include <stdexcept>
namespace elementalEngine::RHI {
DX12ComputePipeline::DX12ComputePipeline(DX12Device &device) : device(device) {
  createPipeline();
}

DX12ComputePipeline::~DX12ComputePipeline() {}

void DX12ComputePipeline::createPipeline() {
  auto csBytecode = Core::readFile("build/compute.dxil");

  D3D12_ROOT_PARAMETER rootParam{};
  rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
  rootParam.Descriptor.ShaderRegister = 0; // match u0
  rootParam.Descriptor.RegisterSpace = 0;
  rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
  rootSigDesc.NumParameters = 1;
  rootSigDesc.pParameters = &rootParam;
  rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;
  if (FAILED(D3D12SerializeRootSignature(
          &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
    throw std::runtime_error("Failed to serialize compute root signature!");
  }

  if (FAILED(device.getD3D12Device()->CreateRootSignature(
          0, signature->GetBufferPointer(), signature->GetBufferSize(),
          IID_PPV_ARGS(&rootSignature)))) {
    throw std::runtime_error("Failed to create compute root signature!");
  }

  D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.pRootSignature = rootSignature.Get();
  psoDesc.CS = {csBytecode.data(), csBytecode.size()};

  if (FAILED(device.getD3D12Device()->CreateComputePipelineState(
          &psoDesc, IID_PPV_ARGS(&pipelineState)))) {
    throw std::runtime_error("Failed to create DX12 Compute Pipeline State!");
  }
}

} // namespace elementalEngine::RHI