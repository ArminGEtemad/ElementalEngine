#include "DX12Pipeline.hpp"
#include "FileHandling.hpp"

#include <directx/d3d12.h>
#include <directx/dxgiformat.h>
#include <stdexcept>
#include <stdlib.h>

namespace elementalEngine::RHI {
DX12Pipeline::DX12Pipeline(DX12Device &device,
                           const std::string &vertexShaderName,
                           const std::string &fragmentShaderName)
    : device(device) {
  createPipeline(vertexShaderName, fragmentShaderName);
}

DX12Pipeline::~DX12Pipeline() {}

void DX12Pipeline::createPipeline(const std::string &vertexShaderName,
                                  const std::string &fragmentShaderName) {
  std::string vertFilepath = "build/" + vertexShaderName + ".dxil";
  std::string fragFilepath = "build/" + fragmentShaderName + ".dxil";
  auto vsBytecode = Core::readFile(vertFilepath);
  auto fsBytecode = Core::readFile(fragFilepath);

  // uniform buffer
  // constants from the SimConfig
  D3D12_ROOT_PARAMETER constParam{};
  constParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  constParam.Constants.ShaderRegister = 0; // match b0
  constParam.Constants.RegisterSpace = 0;
  constParam.Constants.Num32BitValues = sizeof(SimConfig) / 4;
  constParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // 1 read texture
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
  // ----------------------DUMMY-----------------------------
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
  // --------------------------------------------------------

  // 3 read texture
  D3D12_DESCRIPTOR_RANGE rangeT3{};
  rangeT3.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  rangeT3.NumDescriptors = 1;
  rangeT3.BaseShaderRegister = 3;
  rangeT3.RegisterSpace = 0;
  rangeT3.OffsetInDescriptorsFromTableStart = 0;

  D3D12_ROOT_PARAMETER t3Param{};
  t3Param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  t3Param.DescriptorTable.NumDescriptorRanges = 1;
  t3Param.DescriptorTable.pDescriptorRanges = &rangeT3;
  t3Param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // combine
  D3D12_ROOT_PARAMETER rootParams[] = {constParam, t1Param, t2Param, t3Param};

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
  rootSigDesc.NumParameters = _countof(rootParams);
  rootSigDesc.pParameters = rootParams;
  rootSigDesc.NumStaticSamplers = 0;
  rootSigDesc.pStaticSamplers = nullptr;
  rootSigDesc.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;
  if (FAILED(D3D12SerializeRootSignature(
          &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error))) {
    throw std::runtime_error("Failed to serialize graphics root signature!");
  }

  if (FAILED(device.getD3D12Device()->CreateRootSignature(
          0, signature->GetBufferPointer(), signature->GetBufferSize(),
          IID_PPV_ARGS(&rootSignature)))) {
    throw std::runtime_error("Failed to create graphics root signature!");
  }

  D3D12_RASTERIZER_DESC rasterizerDesc{};
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
  rasterizerDesc.FrontCounterClockwise = FALSE;
  rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  rasterizerDesc.DepthClipEnable = TRUE;
  rasterizerDesc.MultisampleEnable = FALSE;
  rasterizerDesc.AntialiasedLineEnable = FALSE;
  rasterizerDesc.ForcedSampleCount = 0;
  rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  D3D12_BLEND_DESC blendDesc{};
  blendDesc.AlphaToCoverageEnable = FALSE;
  blendDesc.IndependentBlendEnable = FALSE;
  blendDesc.RenderTarget[0].BlendEnable = FALSE;
  blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
  inputLayoutDesc.pInputElementDescs = nullptr;
  inputLayoutDesc.NumElements = 0;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.InputLayout = inputLayoutDesc;
  psoDesc.pRootSignature = rootSignature.Get();
  psoDesc.VS = {vsBytecode.data(), vsBytecode.size()};
  psoDesc.PS = {fsBytecode.data(), fsBytecode.size()};
  psoDesc.RasterizerState = rasterizerDesc;
  psoDesc.BlendState = blendDesc;
  psoDesc.DepthStencilState.DepthEnable = FALSE;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
  psoDesc.SampleDesc.Count = 1;

  if (FAILED(device.getD3D12Device()->CreateGraphicsPipelineState(
          &psoDesc, IID_PPV_ARGS(&pipelineState)))) {
    throw std::runtime_error("Failed to create DX12 Pipeline State!");
  }
}

} // namespace elementalEngine::RHI