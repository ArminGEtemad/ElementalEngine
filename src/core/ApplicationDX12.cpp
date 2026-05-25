#define GLFW_EXPOSE_NATIVE_WIN32
#include "ApplicationDX12.hpp"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <combaseapi.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <intsafe.h>
#include <iostream>
#include <stdexcept>
#include <winerror.h>
#include <wrl/client.h>

namespace elementalEngine {
ApplicationDX12::ApplicationDX12() { initDX12(); }
ApplicationDX12::~ApplicationDX12() {
  waitForGPU();
  CloseHandle(fenceEvent);
}

void ApplicationDX12::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
    drawFrame();
  }
}

void ApplicationDX12::initDX12() {
  enableDebugLayer();
  createFactory();
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandQueue();
  createSwapchain();
  createRenderTargetViews();
  createCommandList();
  createSyncObjects();
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

// command Queue
void ApplicationDX12::createCommandQueue() {
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

// swapchain
void ApplicationDX12::createSwapchain() {
  DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
  swapchainDesc.Width = WIDTH;
  swapchainDesc.Height = HEIGHT;
  swapchainDesc.BufferCount = FrameCount;
  swapchainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // TODO check the VK format
  swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchainDesc.SampleDesc.Count = 1;
  swapchainDesc.SampleDesc.Quality = 0;
  swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchainDesc.Scaling = DXGI_SCALING_NONE;

  ComPtr<IDXGISwapChain1> tempSwapchain;
  HWND hwnd = glfwGetWin32Window(window.getGLFWwindow());

  if (FAILED(factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd,
                                             &swapchainDesc, nullptr, nullptr,
                                             &tempSwapchain))) {
    throw std::runtime_error("Failed to create DXGI Swapchain!");
  }

  tempSwapchain.As(&swapchain);
}

void ApplicationDX12::createRenderTargetViews() {
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  rtvHeapDesc.NumDescriptors = FrameCount;
  device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

  rtvDescriptorSize =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(
      rtvHeap->GetCPUDescriptorHandleForHeapStart());
  for (UINT i = 0; i < FrameCount; i++) {
    swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
    device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
    rtvHandle.ptr += rtvDescriptorSize;
  }
}

void ApplicationDX12::createCommandList() {
  if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            IID_PPV_ARGS(&commandAllocator)))) {
    throw std::runtime_error("Failed to create Command Allocator!");
  }

  if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       commandAllocator.Get(), nullptr,
                                       IID_PPV_ARGS(&commandList)))) {
    throw std::runtime_error("Failed to create Command List!");
  }

  commandList->Close();
}

void ApplicationDX12::createSyncObjects() {
  if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                 IID_PPV_ARGS(&fence)))) {
    throw std::runtime_error("Failed to create Fence!");
  }

  fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fenceEvent == nullptr) {
    throw std::runtime_error("Failed to create Fence Event!");
  }

  frameIndex = swapchain->GetCurrentBackBufferIndex();
}

void ApplicationDX12::populateCommandList() {
  commandAllocator->Reset();
  commandList->Reset(commandAllocator.Get(), nullptr);

  D3D12_TEXTURE_BARRIER toRenderTargetBarrier{};
  toRenderTargetBarrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
  toRenderTargetBarrier.SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET;
  toRenderTargetBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
  toRenderTargetBarrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
  toRenderTargetBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_PRESENT;
  toRenderTargetBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
  toRenderTargetBarrier.pResource = renderTargets[frameIndex].Get();
  toRenderTargetBarrier.Subresources = {0xffffffff, 0, 0, 0, 0, 0};

  D3D12_BARRIER_GROUP barrierGroup1{};
  barrierGroup1.Type = D3D12_BARRIER_TYPE_TEXTURE;
  barrierGroup1.NumBarriers = 1;
  barrierGroup1.pTextureBarriers = &toRenderTargetBarrier;

  commandList->Barrier(1, &barrierGroup1);

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
      rtvHeap->GetCPUDescriptorHandleForHeapStart();
  rtvHandle.ptr += static_cast<SIZE_T>(frameIndex) * rtvDescriptorSize;

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  const float clearColor[] = {0.01f, 0.01f, 0.1f, 1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  D3D12_TEXTURE_BARRIER toPresentBarrier = toRenderTargetBarrier;
  toPresentBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
  toPresentBarrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
  toPresentBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
  toPresentBarrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
  toPresentBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
  toPresentBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;

  D3D12_BARRIER_GROUP barrierGroup2{};
  barrierGroup2.Type = D3D12_BARRIER_TYPE_TEXTURE;
  barrierGroup2.NumBarriers = 1;
  barrierGroup2.pTextureBarriers = &toPresentBarrier;

  commandList->Barrier(1, &barrierGroup2);

  if (FAILED(commandList->Close())) {
    throw std::runtime_error("Failed to close Command List!");
  }
}

void ApplicationDX12::waitForGPU() {
  const UINT64 fenceToWaitFor = fenceValue;
  commandQueue->Signal(fence.Get(), fenceToWaitFor);
  fenceValue++;

  if (fence->GetCompletedValue() < fenceToWaitFor) {
    fence->SetEventOnCompletion(fenceToWaitFor, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
  }

  frameIndex = swapchain->GetCurrentBackBufferIndex();
}

void ApplicationDX12::drawFrame() {
  populateCommandList();

  ID3D12CommandList *ppCommandLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  if (FAILED(swapchain->Present(1, 0))) {
    throw std::runtime_error("Swapchain failed to present!");
  }

  waitForGPU();
}

} // namespace elementalEngine
