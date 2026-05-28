#pragma once

// add header files
#include "Device.hpp"
#include "Window.hpp"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

namespace elementalEngine::RHI {
class DX12Device : public Device {
public:
  DX12Device(const DeviceConfig &config, WindowHandling &window);
  ~DX12Device() override;

  GraphicsAPI getAPI() const override { return GraphicsAPI::DirectX12; }
  void waitIdle() override;

  // getter functions
  IDXGIFactory4 *getFactory() const { return factory.Get(); }
  ID3D12Device8 *getD3D12Device() const { return device.Get(); }
  ID3D12CommandQueue *getCommandQueue() const { return commandQueue.Get(); }

private:
  ComPtr<IDXGIFactory4> factory;
  ComPtr<ID3D12Debug> debugController;
  ComPtr<ID3D12Debug1> debugController1;
  ComPtr<IDXGIAdapter1> physicalDevice;
  ComPtr<ID3D12Device8> device;
  ComPtr<ID3D12CommandQueue> commandQueue;

  void createFactory();
  void enableDebugLayer(bool enableGPUValidation);
  void pickPhysicalDevice(); // adapter
  void createLogicalDevice();
  void createCommandQueue();
};
} // namespace elementalEngine::RHI