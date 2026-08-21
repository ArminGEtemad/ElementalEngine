#pragma once

#include "Buffer.hpp"
#include "DX12Device.hpp"
#include "RHICommon.hpp"
#include <D3D12MemAlloc.h>
#include <cstddef>

namespace elementalEngine::RHI {
class DX12Buffer : public Buffer {
public:
  DX12Buffer(DX12Device &device, size_t size, BufferUsage usage,
             MemoryProperty memoryProperty);
  ~DX12Buffer() override;

  size_t getSize() const override { return size; }
  BufferUsage getBufferUsage() const override { return usage; }
  MemoryProperty getMemoryProperty() const override { return memoryProperty; }
  ID3D12Resource *getResource() const { return allocation->GetResource(); }
  void *map() override;
  void unmap() override;

private:
  DX12Device &device;
  size_t size;
  BufferUsage usage;
  MemoryProperty memoryProperty;

  ComPtr<D3D12MA::Allocation> allocation;
};

} // namespace elementalEngine::RHI