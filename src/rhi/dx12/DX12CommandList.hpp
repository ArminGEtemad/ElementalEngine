#pragma once

#include "CommandList.hpp"
#include "DX12Device.hpp"
#include "Swapchain.hpp"

namespace elementalEngine::RHI {
class DX12CommandList : public CommandList {
public:
  explicit DX12CommandList(DX12Device &device);
  ~DX12CommandList() override;

  void begin() override;
  void end() override;
  void beginRendering(Swapchain &swapchain) override;
  void endRendering(Swapchain &swapchain) override;

  void setViewport(float x, float y, float width, float height) override;
  void setScissor(int32_t x, int32_t y, uint32_t width,
                  uint32_t height) override;

  void bindPipeline(Pipeline &pipeline) override;
  void bindVertexBuffer(Buffer *buffer, size_t stride) override;

  void bindComputePipeline(ComputePipeline &pipeline) override;
  void pushConstants(uint32_t offset, uint32_t size, const void *data) override;
  void bindStorageBuffer(uint32_t bindingSlot, Buffer *buffer) override;

  void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                uint32_t groupCountZ) override;
  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance) override;

  ID3D12GraphicsCommandList7 *getNativeCommandList() {
    return commandList.Get();
  }

private:
  DX12Device &device;
  ComPtr<ID3D12CommandAllocator> commandAllocator;
  ComPtr<ID3D12GraphicsCommandList7> commandList;
};
} // namespace elementalEngine::RHI