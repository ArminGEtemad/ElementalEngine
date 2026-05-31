#pragma once

// add header files
#include "CommandList.hpp"
#include "Device.hpp"
#include "Window.hpp"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <memory>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace elementalEngine::RHI {
class DX12Device : public Device {
public:
  DX12Device(const DeviceConfig &config, WindowHandling &window);
  ~DX12Device() override;

  GraphicsAPI getAPI() const override { return GraphicsAPI::DirectX12; }
  void waitIdle() override;
  void submit(CommandList *commandList, Swapchain *swapchain) override;
  std::unique_ptr<Swapchain> createSwapchain(WindowHandling &window) override;
  std::unique_ptr<CommandList> createCommandList() override;

  // getter functions
  IDXGIFactory4 *getFactory() const { return factory.Get(); }
  ID3D12Device8 *getD3D12Device() const { return device.Get(); }
  ID3D12CommandQueue *getCommandQueue() const { return commandQueue.Get(); }
  void waitForGPU();

private:
  ComPtr<IDXGIFactory4> factory;
  ComPtr<ID3D12Debug> debugController;
  ComPtr<ID3D12Debug1> debugController1;
  ComPtr<IDXGIAdapter1> physicalDevice;
  ComPtr<ID3D12Device8> device;
  ComPtr<ID3D12CommandQueue> commandQueue;
  ComPtr<ID3D12Fence> fence;
  UINT64 fenceValue = 0;
  HANDLE fenceEvent;

  void createFactory();
  void enableDebugLayer(bool enableGPUValidation);
  void pickPhysicalDevice(); // adapter
  void createLogicalDevice();
  void createCommandQueue();
  void createSyncObjects();
};
} // namespace elementalEngine::RHI