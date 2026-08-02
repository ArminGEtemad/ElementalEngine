#pragma once

#include "DX12Device.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"

namespace elementalEngine::RHI {
class DX12Swapchain : public Swapchain {
public:
  DX12Swapchain(DX12Device &device, WindowHandling &window);
  ~DX12Swapchain() override;

  void present() override;
  void acquireNextImage() override;
  uint32_t getCurrentFrameIndex() const override;
  ID3D12Resource *getRenderTarget(uint32_t index) const {
    return renderTargets[index].Get();
  }
  // get the handle for the commandList
  D3D12_CPU_DESCRIPTOR_HANDLE getRTVHandle(uint32_t index) const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * rtvDescriptorSize;
    return handle;
  }

private:
  DX12Device &device;
  static constexpr UINT FrameCount = 2; // double buffering swapchain
  uint32_t width = 0;
  uint32_t height = 0;

  ComPtr<IDXGISwapChain3> swapchain;
  ComPtr<ID3D12DescriptorHeap> rtvHeap;
  ComPtr<ID3D12Resource> renderTargets[FrameCount];
  UINT rtvDescriptorSize;

  void createSwapchain(WindowHandling &window);
  void createRenderTargetViews();
};
} // namespace elementalEngine::RHI