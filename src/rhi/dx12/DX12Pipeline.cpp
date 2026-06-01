#include "DX12Pipeline.hpp"
#include "FileHandling.hpp"

#include <stdexcept>

namespace elementalEngine::RHI {
DX12Pipeline::DX12Pipeline(DX12Device &device) : device(device) {
  createPipeline();
}

DX12Pipeline::~DX12Pipeline() {}

void DX12Pipeline::createPipeline() {
  auto vsBytecode = Core::readFile("build/triangle_vs.dxil");
  auto psBytecode = Core::readFile("build/triangle_ps.dxil");

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
  rootSigDesc.NumParameters = 0;
  rootSigDesc.pParameters = nullptr;
  rootSigDesc.NumStaticSamplers = 0;
  rootSigDesc.pStaticSamplers = nullptr;
  rootSigDesc.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;
  D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
                              &signature, &error);
  device.getD3D12Device()->CreateRootSignature(0, signature->GetBufferPointer(),
                                               signature->GetBufferSize(),
                                               IID_PPV_ARGS(&rootSignature));

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

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.InputLayout = {nullptr, 0};
  psoDesc.pRootSignature = rootSignature.Get();
  psoDesc.VS = {vsBytecode.data(), vsBytecode.size()};
  psoDesc.PS = {psBytecode.data(), psBytecode.size()};
  psoDesc.RasterizerState = rasterizerDesc;
  psoDesc.BlendState = blendDesc;
  psoDesc.DepthStencilState.DepthEnable = FALSE;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
  psoDesc.SampleDesc.Count = 1;

  if (FAILED(device.getD3D12Device()->CreateGraphicsPipelineState(
          &psoDesc, IID_PPV_ARGS(&pipelineState)))) {
    throw std::runtime_error("Failed to create DX12 Pipeline State!");
  }
}

} // namespace elementalEngine::RHI