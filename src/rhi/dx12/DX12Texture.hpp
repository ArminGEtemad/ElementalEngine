#pragma once

#include "DX12Device.hpp"
#include "Texture.hpp"
#include <D3D12MemAlloc.h>

namespace elementalEngine::RHI {
class DX12Texture : public Texture {
public:
  DX12Texture(DX12Device &device, uint32_t width, uint32_t height,
              TextureFormat format, TextureUsage usage);
  ~DX12Texture() override;

  uint32_t getWidth() const override { return width; }
  uint32_t getHeight() const override { return height; }
  TextureFormat getFormat() const override { return format; }
  TextureUsage getUsage() const override { return usage; }
  uint32_t getSrvSlot() const { return srvSlot; }
  uint32_t getUavSlot() const { return uavSlot; }

  ID3D12Resource *getResource() const { return allocation->GetResource(); }
  DXGI_FORMAT getDxgiFormat() const { return dxgiFormat; }

private:
  DX12Device &device;
  uint32_t width;
  uint32_t height;
  TextureFormat format;
  TextureUsage usage;
  uint32_t srvSlot = ~0u;
  uint32_t uavSlot = ~0u;

  DXGI_FORMAT dxgiFormat;
  ComPtr<D3D12MA::Allocation> allocation;

  // helper functions
  DXGI_FORMAT mapFormat(TextureFormat format);
  D3D12_RESOURCE_FLAGS mapUsage(TextureUsage usage);
};

} // namespace elementalEngine::RHI