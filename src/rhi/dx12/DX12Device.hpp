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
};
} // namespace elementalEngine::RHI