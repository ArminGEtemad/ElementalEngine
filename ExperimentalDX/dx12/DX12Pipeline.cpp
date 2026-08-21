#include "DX12Pipeline.hpp"
#include "FileHandling.hpp"
#include "Pipeline.hpp"
#include <directx/d3d12.h>
#include <directx/dxgiformat.h>
#include <intsafe.h>
#include <stdexcept>
#include <stdlib.h>
#include <vector>

namespace elementalEngine::RHI {
// map the visibility from DX12 syntax to my RHI
static D3D12_SHADER_VISIBILITY mapShaderStageVisibility(ShaderStage stage) {
  bool hasVertex = (stage & ShaderStage::Vertex);
  bool hasFragment = (stage & ShaderStage::Fragment);
  if (hasVertex && hasFragment)
    return D3D12_SHADER_VISIBILITY_ALL;
  if (hasVertex)
    return D3D12_SHADER_VISIBILITY_VERTEX;
  if (hasFragment)
    return D3D12_SHADER_VISIBILITY_PIXEL;
  return D3D12_SHADER_VISIBILITY_ALL;
}

DX12Pipeline::DX12Pipeline(DX12Device &device,
                           const std::string &vertexShaderName,
                           const std::string &fragmentShaderName,
                           const PipelineConfig &config)
    : device(device) {
  createPipeline(vertexShaderName, fragmentShaderName, config);
}

DX12Pipeline::~DX12Pipeline() {}

void DX12Pipeline::createPipeline(const std::string &vertexShaderName,
                                  const std::string &fragmentShaderName,
                                  const PipelineConfig &config) {

  // All the compiled shaders live inside the build folder
  std::string vertFilepath = "build/" + vertexShaderName + ".dxil";
  std::string fragFilepath = "build/" + fragmentShaderName + ".dxil";
  auto vsBytecode = Core::readFile(vertFilepath);
  auto fsBytecode = Core::readFile(fragFilepath);

  // dynamically put everything into root params
  std::vector<D3D12_ROOT_PARAMETER> rootParams;
  // dynamically setup the bindings
  std::vector<D3D12_DESCRIPTOR_RANGE> descRanges(config.bindings.size());
  std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplerDescs;
  size_t rangeIdx = 0;

  if (config.pushConstants.size > 0) {
    D3D12_ROOT_PARAMETER constParam{};
    constParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    constParam.Constants.ShaderRegister = 0;
    constParam.Constants.Num32BitValues = config.pushConstants.size / 4;
    constParam.ShaderVisibility =
        mapShaderStageVisibility(config.pushConstants.stage);

    pushConstantRootIndex = static_cast<UINT>(rootParams.size());
    rootParams.push_back(constParam);
  }

  for (const auto &binding : config.bindings) {
    if (binding.type == DescriptorType::Sampler) {
      D3D12_STATIC_SAMPLER_DESC sampler{};
      // TODO many of the parameters are hardcoded here and need to NOT be
      // hardcoded at somepoint not the priority until I need them to be handled
      // separately
      sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
      sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      sampler.MipLODBias = 0;
      sampler.MaxAnisotropy = 0;
      sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
      sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      sampler.MinLOD = 0.0f;
      sampler.MaxLOD = D3D12_FLOAT32_MAX;
      sampler.ShaderRegister = binding.bindingSlot;
      sampler.RegisterSpace = 0;
      sampler.ShaderVisibility = mapShaderStageVisibility(binding.stage);
      staticSamplerDescs.push_back(sampler);
      continue;
    }
    D3D12_ROOT_PARAMETER param{};
    param.ShaderVisibility = mapShaderStageVisibility(binding.stage);

    // different kinds of memory
    if (binding.type == DescriptorType::UniformBuffer) {
      param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // buffer view
      param.Descriptor.ShaderRegister = binding.bindingSlot;
      param.Descriptor.RegisterSpace = 0;
    } else if (binding.type == DescriptorType::StorageBuffer) {
      param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
      param.Descriptor.ShaderRegister = binding.bindingSlot;
      param.Descriptor.RegisterSpace = 0;
    } else if (binding.type == DescriptorType::SampledImage ||
               binding.type == DescriptorType::StorageImage) {
      // texture we have to define the tables
      param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

      auto &range = descRanges[rangeIdx++];
      range.RangeType = (binding.type == DescriptorType::SampledImage)
                            ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV
                            : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      range.NumDescriptors = binding.count;
      range.BaseShaderRegister = binding.bindingSlot;
      range.RegisterSpace = 0;
      range.OffsetInDescriptorsFromTableStart = 0;
      param.DescriptorTable.NumDescriptorRanges = 1;
      param.DescriptorTable.pDescriptorRanges = &range;
    }
    slotToRootIndex[binding.bindingSlot] = static_cast<UINT>(rootParams.size());
    rootParams.push_back(param);
  }

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
  rootSigDesc.NumParameters = static_cast<UINT>(rootParams.size());
  rootSigDesc.pParameters = rootParams.empty() ? nullptr : rootParams.data();
  rootSigDesc.NumStaticSamplers = static_cast<UINT>(staticSamplerDescs.size());
  rootSigDesc.pStaticSamplers =
      staticSamplerDescs.empty() ? nullptr : staticSamplerDescs.data();
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