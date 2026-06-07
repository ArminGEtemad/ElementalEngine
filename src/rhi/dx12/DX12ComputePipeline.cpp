#include "DX12ComputePipeline.hpp"
#include "DX12Device.hpp"
#include "FileHandling.hpp"
#include <combaseapi.h>
#include <directx/d3d12.h>
#include <intsafe.h>
#include <stdexcept>
#include <stdlib.h>
namespace elementalEngine::RHI {
DX12ComputePipeline::DX12ComputePipeline(DX12Device &device) : device(device) {
  createPipeline();
}

DX12ComputePipeline::~DX12ComputePipeline() {}

void DX12ComputePipeline::createPipeline() {
  auto csBytecode = Core::readFile("build/advection.dxil");

  // constants from the SimConfig
  D3D12_ROOT_PARAMETER constParam{};
  constParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  constParam.Constants.ShaderRegister = 0; // match b0
  constParam.Constants.RegisterSpace = 0;
  constParam.Constants.Num32BitValues = sizeof(SimConfig) / 4;
  constParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // storage read density
  D3D12_ROOT_PARAMETER t1Param{};
  t1Param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
  t1Param.Descriptor.ShaderRegister = 1; // mathc t1
  t1Param.Descriptor.RegisterSpace = 0;
  t1Param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // storage read velocity
  D3D12_ROOT_PARAMETER t2Param{};
  t2Param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
  t2Param.Descriptor.ShaderRegister = 2; // mathc t2
  t2Param.Descriptor.RegisterSpace = 0;
  t2Param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // storage read write density
  D3D12_ROOT_PARAMETER u3Param{};
  u3Param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
  u3Param.Descriptor.ShaderRegister = 3; // mathc u3
  u3Param.Descriptor.RegisterSpace = 0;
  u3Param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // combine
  D3D12_ROOT_PARAMETER rootParams[] = {constParam, t1Param, t2Param, u3Param};

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
  rootSigDesc.NumParameters = _countof(rootParams);
  rootSigDesc.pParameters = rootParams;
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