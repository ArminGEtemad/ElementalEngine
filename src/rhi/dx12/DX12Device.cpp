#include "DX12Device.hpp"
#include "DX12Buffer.hpp"
#include "DX12CommandList.hpp"
#include "DX12ComputePipeline.hpp"
#include "DX12Pipeline.hpp"
#include "DX12Swapchain.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"
#include <D3D12MemAlloc.h>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace elementalEngine::RHI {

DX12Device::DX12Device(const DeviceConfig &config, WindowHandling &window) {
  if (config.enableValidationLayers) {
    enableDebugLayer(config.enableGPUAssistedValidatioLayer);
  }
  createFactory();
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandQueue();
  createSyncObjects();
  createAllocator();
}

DX12Device::~DX12Device() { CloseHandle(fenceEvent); }

void DX12Device::waitIdle() { waitForGPU(); }

std::unique_ptr<Swapchain> DX12Device::createSwapchain(WindowHandling &window) {
  return std::make_unique<DX12Swapchain>(*this, window);
}

std::unique_ptr<CommandList> DX12Device::createCommandList() {
  return std::make_unique<DX12CommandList>(*this);
}

std::unique_ptr<Pipeline>
DX12Device::createPipeline(const std::string &vertexShaderName,
                           const std::string &fragmentShaderName) {
  return std::make_unique<DX12Pipeline>(*this, vertexShaderName,
                                        fragmentShaderName);
}

std::unique_ptr<Pipeline>
DX12Device::createComputePipeline(const std::string &computeShaderName) {
  return std::make_unique<DX12ComputePipeline>(*this, computeShaderName);
}

std::unique_ptr<Buffer> DX12Device::createBuffer(size_t size, BufferUsage usage,
                                                 MemoryProperty memory) {
  return std::make_unique<DX12Buffer>(*this, size, usage, memory);
}

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
  if (FAILED(D3D12CreateDevice(physicalDevice.Get(), D3D_FEATURE_LEVEL_12_2,
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

void DX12Device::submit(CommandList *commandList, Swapchain *swapchain) {
  auto *dxCmdList = static_cast<DX12CommandList *>(commandList);

  ID3D12CommandList *ppCommandLists[] = {dxCmdList->getNativeCommandList()};
  commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void DX12Device::createSyncObjects() {
  if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                 IID_PPV_ARGS(&fence)))) {
    throw std::runtime_error("Failed to create Fence!");
  }

  fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fenceEvent == nullptr) {
    throw std::runtime_error("Failed to create Fence Event!");
  }
}

void DX12Device::waitForGPU() {
  const UINT64 fenceToWaitFor = fenceValue;
  commandQueue->Signal(fence.Get(), fenceToWaitFor);
  fenceValue++;

  if (fence->GetCompletedValue() < fenceToWaitFor) {
    fence->SetEventOnCompletion(fenceToWaitFor, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
  }
}

void DX12Device::createAllocator() {
  D3D12MA::ALLOCATOR_DESC allocDesc{};
  allocDesc.pDevice = device.Get();
  allocDesc.pAdapter = physicalDevice.Get();

  if (FAILED(D3D12MA::CreateAllocator(&allocDesc, &allocator))) {
    throw std::runtime_error("Failed to create D3D12MA Memory Allocator");
  }
}

} // namespace elementalEngine::RHI