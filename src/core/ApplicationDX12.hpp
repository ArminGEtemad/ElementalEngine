#pragma once

// header files
#include "Window.hpp"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
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
  // --- initialization ---
  WindowHandling window{WIDTH, HEIGHT, "Elemental Engine - DX12"};
  // - DX12 Monolith -
  ComPtr<IDXGIFactory4> factory;
  ComPtr<ID3D12Debug1> debugController;
  ComPtr<IDXGIAdapter1> physicalDevice;
  ComPtr<ID3D12Device> device;

  void initDX12();
  void createFactory();
  void enableDebugLayer();
  void pickPhysicalDevice(); // adapter
  void createLogicalDevice();
};
} // namespace elementalEngine