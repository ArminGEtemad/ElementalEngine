#include "DX12Device.hpp"
#include "Window.hpp"
#include <iostream>

namespace elementalEngine::RHI {

DX12Device::DX12Device(const DeviceConfig &config, WindowHandling &window) {
  if (config.enableValidationLayers) {
    enableDebugLayer(config.enableGPUAssistedValidatioLayer);
  }
  createFactory();
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandQueue();
}

DX12Device::~DX12Device() {}

void DX12Device::waitIdle() {}

void DX12Device::enableDebugLayer(bool enableGPUValidation) {

  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();
    std::cout << "D3D12 Core Debug Layer Enabled.\n";

    if (enableGPUValidation) {
      if (SUCCEEDED(debugController.As(&debugController1))) {
        debugController1->SetEnableGPUBasedValidation(true);
        std::cout << "D3D12 GPU-Based Validation Enabled.\n";
      }
    }
  } else {
    std::cerr << "Direct3D Debug Device is not available. Are Graphics Tools "
                 "installed?\n";
  }
}

// instance
void DX12Device::createFactory() {
  UINT factoryFlags = 0;

  if (debugController != nullptr) {
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
  }

  if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)))) {
    throw std::runtime_error("Failed to create DXGI Factory!");
  }
}

// find the physical device
void DX12Device::pickPhysicalDevice() {
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
void DX12Device::createLogicalDevice() {
  if (FAILED(D3D12CreateDevice(physicalDevice.Get(), D3D_FEATURE_LEVEL_12_1,
                               IID_PPV_ARGS(&device)))) {
    throw std::runtime_error("Failed to create logical DirectX 12 Device!");
  }
}

// command Queue
void DX12Device::createCommandQueue() {
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.NodeMask = 0;

  if (FAILED(device->CreateCommandQueue(&queueDesc,
                                        IID_PPV_ARGS(&commandQueue)))) {
    throw std::runtime_error("Failed to create DX12 Command Queue!");
  }
}
} // namespace elementalEngine::RHI