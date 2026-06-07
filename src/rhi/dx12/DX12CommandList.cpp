#include "DX12CommandList.hpp"
#include "DX12Buffer.hpp"
#include "DX12ComputePipeline.hpp"
#include "DX12Device.hpp"
#include "DX12Pipeline.hpp"
#include "DX12Swapchain.hpp"
#include "Swapchain.hpp"
#include <cstdint>
#include <stdexcept>

namespace elementalEngine::RHI {
DX12CommandList::DX12CommandList(DX12Device &device) : device(device) {
  // creates the allocator
  if (FAILED(device.getD3D12Device()->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)))) {
    throw std::runtime_error("Failed to create Command Allocator!");
  }

  // create the command list
  if (FAILED(device.getD3D12Device()->CreateCommandList(
          0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
          IID_PPV_ARGS(&commandList)))) {
    throw std::runtime_error("Failed to create Command List!");
  }

  commandList->Close();
}
DX12CommandList::~DX12CommandList() {}

// reset
void DX12CommandList::begin() {
  commandAllocator->Reset();

  if (FAILED(commandList->Reset(commandAllocator.Get(), nullptr))) {
    throw std::runtime_error("Failed to reset DX12 Command List!");
  }
}

// ending and ready to submit
void DX12CommandList::end() {
  if (FAILED(commandList->Close())) {
    throw std::runtime_error("Failed to close Command List!");
  }
}

void DX12CommandList::beginRendering(Swapchain &swapchain) {
  auto &dx12Swapchain = static_cast<DX12Swapchain &>(swapchain);
  uint32_t frameIndex = dx12Swapchain.getCurrentFrameIndex();
  ID3D12Resource *renderTarget = dx12Swapchain.getRenderTarget(frameIndex);

  D3D12_TEXTURE_BARRIER toRenderTargetBarrier{};
  toRenderTargetBarrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
  toRenderTargetBarrier.SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET;
  toRenderTargetBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
  toRenderTargetBarrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
  toRenderTargetBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_PRESENT;
  toRenderTargetBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
  toRenderTargetBarrier.pResource = renderTarget;
  toRenderTargetBarrier.Subresources = {0xffffffff, 0, 0, 0, 0, 0};

  D3D12_BARRIER_GROUP barrierGroup1{};
  barrierGroup1.Type = D3D12_BARRIER_TYPE_TEXTURE;
  barrierGroup1.NumBarriers = 1;
  barrierGroup1.pTextureBarriers = &toRenderTargetBarrier;

  commandList->Barrier(1, &barrierGroup1);

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
      dx12Swapchain.getRTVHandle(frameIndex);

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
  // TODO not hardcoded anymore later
  const float clearColor[] = {0.01f, 0.01f, 0.1f, 1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void DX12CommandList::endRendering(Swapchain &swapchain) {
  auto &dx12Swapchain = static_cast<DX12Swapchain &>(swapchain);
  uint32_t frameIndex = dx12Swapchain.getCurrentFrameIndex();
  ID3D12Resource *renderTarget = dx12Swapchain.getRenderTarget(frameIndex);

  D3D12_TEXTURE_BARRIER toPresentBarrier{};
  toPresentBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
  toPresentBarrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
  toPresentBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
  toPresentBarrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
  toPresentBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
  toPresentBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
  toPresentBarrier.pResource = renderTarget;
  toPresentBarrier.Subresources = {0xffffffff, 0, 0, 0, 0, 0};

  D3D12_BARRIER_GROUP barrierGroup{};
  barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
  barrierGroup.NumBarriers = 1;
  barrierGroup.pTextureBarriers = &toPresentBarrier;

  commandList->Barrier(1, &barrierGroup);
}

void DX12CommandList::setViewport(float x, float y, float width, float height) {
  D3D12_VIEWPORT viewport{};
  viewport.TopLeftX = x;
  viewport.TopLeftY = y;
  viewport.Width = width;
  viewport.Height = height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  commandList->RSSetViewports(1, &viewport);
}

void DX12CommandList::setScissor(int32_t x, int32_t y, uint32_t width,
                                 uint32_t height) {
  D3D12_RECT scissorRect{};
  scissorRect.left = x;
  scissorRect.top = y;
  scissorRect.right = x + width;
  scissorRect.bottom = y + height;
  commandList->RSSetScissorRects(1, &scissorRect);
}

void DX12CommandList::bindPipeline(Pipeline &pipeline) {
  auto &dx12Pipeline = static_cast<DX12Pipeline &>(pipeline);

  commandList->SetPipelineState(dx12Pipeline.getNativePipelineState());
  commandList->SetGraphicsRootSignature(dx12Pipeline.getNativeRootSignature());

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DX12CommandList::bindComputePipeline(ComputePipeline &pipeline) {
  auto &dx12Pipeline = static_cast<DX12ComputePipeline &>(pipeline);

  commandList->SetPipelineState(dx12Pipeline.getNativePipelineState());
  commandList->SetComputeRootSignature(dx12Pipeline.getNativeRootSignature());
}

void DX12CommandList::dispatch(uint32_t groupCountX, uint32_t groupCountY,
                               uint32_t groupCountZ) {
  commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void DX12CommandList::draw(uint32_t vertexCount, uint32_t instanceCount,
                           uint32_t firstVertex, uint32_t firstInstance) {
  commandList->DrawInstanced(vertexCount, instanceCount, firstVertex,
                             firstInstance);
}

void DX12CommandList::bindVertexBuffer(Buffer *buffer, size_t stride) {
  auto *dx12Buffer = static_cast<DX12Buffer *>(buffer);

  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  vertexBufferView.BufferLocation =
      dx12Buffer->getResource()->GetGPUVirtualAddress();
  vertexBufferView.StrideInBytes = static_cast<UINT>(stride);
  vertexBufferView.SizeInBytes = static_cast<UINT>(dx12Buffer->getSize());

  commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
}

void DX12CommandList::pushConstants(uint32_t offset, uint32_t size,
                                    const void *data) {
  commandList->SetComputeRoot32BitConstants(0, size / 4, data, offset / 4);
}

void DX12CommandList::bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) {
  auto *dx12Buffer = static_cast<DX12Buffer *>(buffer);

  // read only
  if (bindingSlot == 1 || bindingSlot == 2) {
    commandList->SetComputeRootShaderResourceView(
        bindingSlot, dx12Buffer->getResource()->GetGPUVirtualAddress());
  }
  // read and write
  else if (bindingSlot == 3) {
    commandList->SetComputeRootUnorderedAccessView(
        bindingSlot, dx12Buffer->getResource()->GetGPUVirtualAddress());
  }
}
} // namespace elementalEngine::RHI