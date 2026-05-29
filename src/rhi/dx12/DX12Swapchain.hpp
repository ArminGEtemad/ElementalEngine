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