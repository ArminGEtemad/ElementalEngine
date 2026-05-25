#pragma once

// header files
#include "Window.hpp"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <vector>
#include <wrl.h>


using Microsoft::WRL::ComPtr;

namespace elementalEngine {
class ApplicationDX12 {
public:
  // TODO for now the window is not resizable
  // change it later after triangle is up
  static constexpr int WIDTH{1000};
  static constexpr int HEIGHT{800};

  ApplicationDX12();
  ~ApplicationDX12();

  // cleaning up
  ApplicationDX12(const ApplicationDX12 &) = delete;
  ApplicationDX12 &operator=(const ApplicationDX12 &) = delete;

  // functions
  void run();

private:
  static constexpr UINT FrameCount = 2; // double buffering swapchain

  // --- initialization ---
  WindowHandling window{WIDTH, HEIGHT, "Elemental Engine - DX12"};
  // - DX12 Monolith -
  ComPtr<IDXGIFactory4> factory;
  ComPtr<ID3D12Debug1> debugController;
  ComPtr<IDXGIAdapter1> physicalDevice;
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12CommandQueue> commandQueue;
  ComPtr<IDXGISwapChain3> swapchain;
  ComPtr<ID3D12DescriptorHeap> rtvHeap;
  ComPtr<ID3D12Resource> renderTargets[FrameCount];
  UINT rtvDescriptorSize;
  ComPtr<ID3D12CommandAllocator> commandAllocator;
  ComPtr<ID3D12GraphicsCommandList7> commandList;
  ComPtr<ID3D12Fence> fence;
  UINT64 fenceValue = 0;
  HANDLE fenceEvent;
  UINT frameIndex = 0;
  ComPtr<ID3D12RootSignature> rootSignature;
  ComPtr<ID3D12PipelineState> pipelineState;

  static std::vector<char>
  readFile(const std::string &filepath); // helper function to read shader

  void initDX12();
  void createFactory();
  void enableDebugLayer();
  void pickPhysicalDevice(); // adapter
  void createLogicalDevice();
  void createCommandQueue();
  void createSwapchain();
  void createRenderTargetViews();
  void createCommandList();
  void createSyncObjects();
  void createGraphicsPipeline();
  void populateCommandList();
  void waitForGPU();
  void drawFrame();
};
} // namespace elementalEngine