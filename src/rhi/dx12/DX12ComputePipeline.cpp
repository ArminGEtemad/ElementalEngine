#include "DX12ComputePipeline.hpp"
#include "DX12Device.hpp"
#include "FileHandling.hpp"
#include <combaseapi.h>
#include <directx/d3d12.h>
#include <intsafe.h>
#include <stdexcept>
#include <stdlib.h>
namespace elementalEngine::RHI {
DX12ComputePipeline::DX12ComputePipeline(DX12Device &device,
                                         const std::string &shaderName)
    : device(device) {
  createPipeline(shaderName);
}

DX12ComputePipeline::~DX12ComputePipeline() {}

void DX12ComputePipeline::createPipeline(const std::string &shaderName) {
  std::string filepath = "build/" + shaderName + ".dxil";
  auto csBytecode = Core::readFile(filepath);

  // constants from the SimConfig
  D3D12_ROOT_PARAMETER constParam{};
  constParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  constParam.Constants.ShaderRegister = 0; // match b0
  constParam.Constants.RegisterSpace = 0;
  constParam.Constants.Num32BitValues = sizeof(SimConfig) / 4;
  constParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // 1 reat texture
  D3D12_DESCRIPTOR_RANGE rangeT1{};
  rangeT1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  rangeT1.NumDescriptors = 1;
  rangeT1.BaseShaderRegister = 1;
  rangeT1.RegisterSpace = 0;
  rangeT1.OffsetInDescriptorsFromTableStart = 0;

  D3D12_ROOT_PARAMETER t1Param{};
  t1Param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  t1Param.DescriptorTable.NumDescriptorRanges = 1;
  t1Param.DescriptorTable.pDescriptorRanges = &rangeT1;
  t1Param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // 2 reat texture
  D3D12_DESCRIPTOR_RANGE rangeT2{};
  rangeT2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  rangeT2.NumDescriptors = 1;
  rangeT2.BaseShaderRegister = 2;
  rangeT2.RegisterSpace = 0;
  rangeT2.OffsetInDescriptorsFromTableStart = 0;

  D3D12_ROOT_PARAMETER t2Param{};
  t2Param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  t2Param.DescriptorTable.NumDescriptorRanges = 1;
  t2Param.DescriptorTable.pDescriptorRanges = &rangeT2;
  t2Param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // 3 write texture
  D3D12_DESCRIPTOR_RANGE rangeU3{};
  rangeU3.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  rangeU3.NumDescriptors = 1;
  rangeU3.BaseShaderRegister = 3;
  rangeU3.RegisterSpace = 0;
  rangeU3.OffsetInDescriptorsFromTableStart = 0;

  D3D12_ROOT_PARAMETER u3Param{};
  u3Param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  u3Param.DescriptorTable.NumDescriptorRanges = 1;
  u3Param.DescriptorTable.pDescriptorRanges = &rangeU3;
  u3Param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // 4 write texture
  D3D12_DESCRIPTOR_RANGE rangeU4{};
  rangeU4.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  rangeU4.NumDescriptors = 1;
  rangeU4.BaseShaderRegister = 4;
  rangeU4.RegisterSpace = 0;
  rangeU4.OffsetInDescriptorsFromTableStart = 0;

  D3D12_ROOT_PARAMETER u4Param{};
  u4Param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  u4Param.DescriptorTable.NumDescriptorRanges = 1;
  u4Param.DescriptorTable.pDescriptorRanges = &rangeU4;
  u4Param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // 5 sampler
  D3D12_STATIC_SAMPLER_DESC sampler{};
  sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.MipLODBias = 0;
  sampler.MaxAnisotropy = 0;
  sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler.MinLOD = 0.0f;
  sampler.MaxLOD = D3D12_FLOAT32_MAX;
  sampler.ShaderRegister = 5;
  sampler.RegisterSpace = 0;
  sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // combine
  D3D12_ROOT_PARAMETER rootParams[] = {constParam, t1Param, t2Param, u3Param,
                                       u4Param};

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
  rootSigDesc.NumParameters = _countof(rootParams);
  rootSigDesc.pParameters = rootParams;
  rootSigDesc.NumStaticSamplers = 1;
  rootSigDesc.pStaticSamplers = &sampler;
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