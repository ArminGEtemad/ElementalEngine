#include "ApplicationDX12.hpp"
#include <GLFW/glfw3.h>
#include <combaseapi.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <dxgi.h>
#include <intsafe.h>
#include <iostream>
#include <stdexcept>
#include <winerror.h>
#include <wrl/client.h>

namespace elementalEngine {
ApplicationDX12::ApplicationDX12() { initDX12(); }
ApplicationDX12::~ApplicationDX12() {
  // TODO cleaning up
}

void ApplicationDX12::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
  }
}

void ApplicationDX12::initDX12() {
  enableDebugLayer();
  createFactory();
  pickPhysicalDevice();
  createLogicalDevice();
}

// debug layout
void ApplicationDX12::enableDebugLayer() {

  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();

    debugController->SetEnableGPUBasedValidation(true);
    std::cout << "D3D12 Debug Layer and GPU-Based Validation Enabled.\n";
  } else {
    std::cerr << "Direct3D Debug Device is not available. Are "
                 "Graphics Tools are installed?\n";
  }
}

// instance
void ApplicationDX12::createFactory() {
  UINT factoryFlags = 0;

  if (debugController != nullptr) {
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
  }

  if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)))) {
    throw std::runtime_error("Failed to create DXGI Factory!");
  }
}

// physical device adapter
void ApplicationDX12::pickPhysicalDevice() {
  ComPtr<IDXGIAdapter1> adapter;

  // find all devices
  for (UINT adapterIndex = 0;
       DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter);
       ++adapterIndex) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    // no software renderer
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      continue;
    }

    if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2,
                                    _uuidof(ID3D12Device), nullptr))) {
      physicalDevice = adapter;

      std::wcout << L"Selected DX12 Hardware Adapter: " << desc.Description
                 << L"\n";
      break;
    }
  }

  if (physicalDevice == nullptr) {
    throw std::runtime_error("Failed to find a GPU suitable for DX12!");
  }
}

// logical device
void ApplicationDX12::createLogicalDevice() {
  if (FAILED(D3D12CreateDevice(physicalDevice.Get(), D3D_FEATURE_LEVEL_12_1,
                               IID_PPV_ARGS(&device)))) {
    throw std::runtime_error("Failed to create logical DirectX 12 Device!");
  }
}

} // namespace elementalEngine
