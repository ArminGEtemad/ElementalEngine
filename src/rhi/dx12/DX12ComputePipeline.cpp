#include "DX12ComputePipeline.hpp"
#include "DX12Device.hpp"
#include "FileHandling.hpp"
#include <combaseapi.h>
#include <directx/d3d12.h>
#include <intsafe.h>
#include <stdexcept>
#include <stdlib.h>
namespace elementalEngine::RHI {

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

DX12ComputePipeline::DX12ComputePipeline(DX12Device &device,
                                         const std::string &shaderName,
                                         const PipelineConfig &config)
    : device(device) {
  createPipeline(shaderName, config);
}

DX12ComputePipeline::~DX12ComputePipeline() {}

void DX12ComputePipeline::createPipeline(const std::string &shaderName,
                                         const PipelineConfig &config) {
  std::string filepath = "build/" + shaderName + ".dxil";
  auto csBytecode = Core::readFile(filepath);

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