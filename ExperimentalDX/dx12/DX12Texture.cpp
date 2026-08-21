#include "DX12Texture.hpp"
#include <D3D12MemAlloc.h>
#include <combaseapi.h>
#include <directx/d3d12.h>
#include <intsafe.h>
#include <stdexcept>

namespace elementalEngine::RHI {
DX12Texture::DX12Texture(DX12Device &device, uint32_t width, uint32_t height,
                         TextureFormat format, TextureUsage usage)
    : device(device), width(width), height(height), format(format),
      usage(usage) {

  dxgiFormat = mapFormat(format);
  D3D12_RESOURCE_FLAGS resouceFlags = mapUsage(usage);

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = width;
  resourceDesc.Height = height;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = dxgiFormat;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // look into it
  resourceDesc.Flags = resouceFlags;

  // memory allocator
  D3D12MA::ALLOCATION_DESC allocDesc{};
  allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

  if (FAILED(device.getAllocator()->CreateResource(
          &allocDesc, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
          &allocation, __uuidof(ID3D12Resource), nullptr))) {
    throw std::runtime_error("Failed to allocate Texture via D3D12MA!");
  }

  if (usage & TextureUsage::ShaderResource) {
    srvSlot = device.allocateDescriptorSlot();

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
        device.getGlobalDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += static_cast<SIZE_T>(srvSlot) * device.getDescriptorSize();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = dxgiFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    device.getD3D12Device()->CreateShaderResourceView(allocation->GetResource(),
                                                      &srvDesc, srvHandle);
  }

  if (usage & TextureUsage::UnorderedAccess) {
    uavSlot = device.allocateDescriptorSlot();

    D3D12_CPU_DESCRIPTOR_HANDLE uavHandle =
        device.getGlobalDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
    uavHandle.ptr += static_cast<SIZE_T>(uavSlot) * device.getDescriptorSize();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = dxgiFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    device.getD3D12Device()->CreateUnorderedAccessView(
        allocation->GetResource(), nullptr, &uavDesc, uavHandle);
  }
}

DX12Texture::~DX12Texture() {}

DXGI_FORMAT DX12Texture::mapFormat(TextureFormat format) {
  switch (format) {
  case TextureFormat::B8G8R8A8_SRGB:
    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
  case TextureFormat::R32_FLOAT:
    return DXGI_FORMAT_R32_FLOAT;
  case TextureFormat::R32G32_FLOAT:
    return DXGI_FORMAT_R32G32_FLOAT;
  default:
    throw std::runtime_error("Unsupported Texture Format in DX12!");
  }
}

D3D12_RESOURCE_FLAGS DX12Texture::mapUsage(TextureUsage usage) {
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
  if (usage & TextureUsage::UnorderedAccess)
    flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if (usage & TextureUsage::RenderTarget)
    flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  return flags;
}

} // namespace elementalEngine::RHI