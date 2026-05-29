#include <dxgiformat.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include "DX12Device.hpp"
#include "DX12Swapchain.hpp"
#include <GLFW/glfw3native.h>
#include <stdexcept>

namespace elementalEngine::RHI {
DX12Swapchain::DX12Swapchain(DX12Device &device, WindowHandling &window)
    : device(device) {
  int w, h;
  glfwGetFramebufferSize(window.getGLFWwindow(), &w, &h);
  width = static_cast<uint32_t>(w);
  height = static_cast<uint32_t>(h);

  createSwapchain(window);
  createRenderTargetViews();
}

DX12Swapchain::~DX12Swapchain() {}

// swapchain
void DX12Swapchain::createSwapchain(WindowHandling &window) {
  DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
  swapchainDesc.Width = width;
  swapchainDesc.Height = height;
  swapchainDesc.BufferCount = FrameCount;
  swapchainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // has to become SRGB later
  swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchainDesc.SampleDesc.Count = 1;
  swapchainDesc.SampleDesc.Quality = 0;
  swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchainDesc.Scaling = DXGI_SCALING_NONE;

  ComPtr<IDXGISwapChain1> tempSwapchain;
  HWND hwnd = glfwGetWin32Window(window.getGLFWwindow());

  if (FAILED(device.getFactory()->CreateSwapChainForHwnd(
          device.getCommandQueue(), hwnd, &swapchainDesc, nullptr, nullptr,
          &tempSwapchain))) {
    throw std::runtime_error("Failed to create DXGI Swapchain!");
  }

  tempSwapchain.As(&swapchain);
}

// target views
void DX12Swapchain::createRenderTargetViews() {
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  rtvHeapDesc.NumDescriptors = FrameCount;
  device.getD3D12Device()->CreateDescriptorHeap(&rtvHeapDesc,
                                                IID_PPV_ARGS(&rtvHeap));

  rtvDescriptorSize = device.getD3D12Device()->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(
      rtvHeap->GetCPUDescriptorHandleForHeapStart());
  for (UINT i = 0; i < FrameCount; i++) {
    swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
    device.getD3D12Device()->CreateRenderTargetView(renderTargets[i].Get(),
                                                    nullptr, rtvHandle);
    rtvHandle.ptr += rtvDescriptorSize;
  }
}

void DX12Swapchain::present() {
  if (FAILED(swapchain->Present(1, 0))) {
    throw std::runtime_error("Swapchain failed to present!");
  }
}

uint32_t DX12Swapchain::getCurrentFrameIndex() const {
  return swapchain->GetCurrentBackBufferIndex();
}

void DX12Swapchain::acquireNextImage() {}

} // namespace elementalEngine::RHI