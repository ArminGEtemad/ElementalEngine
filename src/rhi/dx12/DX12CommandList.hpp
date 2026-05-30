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

  ID3D12GraphicsCommandList7 *getNativeCommandList() {
    return commandList.Get();
  }

private:
  DX12Device &device;
  ComPtr<ID3D12CommandAllocator> commandAllocator;
  ComPtr<ID3D12GraphicsCommandList7> commandList;
};
} // namespace elementalEngine::RHI