#include "DX12Buffer.hpp"
#include "RHICommon.hpp"

#include <D3D12MemAlloc.h>
#include <directx/d3d12.h>
#include <directx/dxgiformat.h>
#include <intsafe.h>
#include <stdexcept>

namespace elementalEngine::RHI {
DX12Buffer::DX12Buffer(DX12Device &device, size_t size, BufferUsage usage,
                       MemoryProperty memoryProperty)
    : device(device), size(size), usage(usage), memoryProperty(memoryProperty) {

  D3D12MA::ALLOCATION_DESC allocDesc{};
  // map memory
  if (memoryProperty == MemoryProperty::CPUAccess) {
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
  } else {
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
  }

  // resource desctiption
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Alignment = 0;
  resourceDesc.Width = size;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  // buffer usage
  if (usage & BufferUsage::Storage) {
    resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }

  // initial state
  D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;
  if (memoryProperty == MemoryProperty::CPUAccess) {
    initState = D3D12_RESOURCE_STATE_GENERIC_READ;
  }

  // create
  if (FAILED(device.getAllocator()->CreateResource(
          &allocDesc, &resourceDesc, initState, nullptr, &this->allocation,
          __uuidof(ID3D12Resource), nullptr))) {
    throw std::runtime_error("Failed to allocate DX12 Buffer via D3D12MA!");
  }
}

DX12Buffer::~DX12Buffer() {}

void *DX12Buffer::map() {
  if (this->memoryProperty != MemoryProperty::CPUAccess) {
    throw std::runtime_error("Cannot map a GPULocal directly!");
  }

  void *mappedData = nullptr;
  D3D12_RANGE readRange{0, 0};
  if (FAILED(
          this->allocation->GetResource()->Map(0, &readRange, &mappedData))) {
    throw std::runtime_error("Failed to map DX12 Buffer!");
  }

  return mappedData;
}

void DX12Buffer::unmap() {
  if (this->memoryProperty != MemoryProperty::CPUAccess)
    return;
  D3D12_RANGE writtenRage{0, this->size};
  this->allocation->GetResource()->Unmap(0, &writtenRage);
}

} // namespace elementalEngine::RHI