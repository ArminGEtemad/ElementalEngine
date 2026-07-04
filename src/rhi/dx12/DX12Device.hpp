#pragma once

// add header files
#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"
#include <D3D12MemAlloc.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <memory>
#include <wrl.h>
#include <wrl/client.h>

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

  std::unique_ptr<Pipeline>
  createPipeline(const std::string &vertexShaderName,
                 const std::string &fragmentShaderName,
                 const PipelineConfig &config) override;

  std::unique_ptr<Pipeline>
  createComputePipeline(const std::string &computeShaderName,
                        const PipelineConfig &config) override;

  std::unique_ptr<Buffer> createBuffer(size_t size, BufferUsage usage,
                                       MemoryProperty memory) override;
  std::unique_ptr<Texture> createTexture(uint32_t gridWidth,
                                         uint32_t gridHeight,
                                         TextureFormat format,
                                         TextureUsage usage) override;
  // getter functions
  IDXGIFactory4 *getFactory() const { return factory.Get(); }
  ID3D12Device8 *getD3D12Device() const { return device.Get(); }
  ID3D12CommandQueue *getCommandQueue() const { return commandQueue.Get(); }
  D3D12MA::Allocator *getAllocator() const { return allocator.Get(); }
  ID3D12DescriptorHeap *getGlobalDescriptorHeap() const {
    return globalSrvUavHeap.Get();
  }
  UINT getDescriptorSize() const { return srvUavDescriptorSize; }
  uint32_t allocateDescriptorSlot();
  void waitForGPU();

private:
  ComPtr<IDXGIFactory4> factory;
  ComPtr<ID3D12Debug> debugController;
  ComPtr<ID3D12Debug1> debugController1;
  ComPtr<IDXGIAdapter1> physicalDevice;
  ComPtr<ID3D12Device8> device;
  ComPtr<ID3D12CommandQueue> commandQueue;
  ComPtr<ID3D12Fence> fence;
  UINT64 fenceValue{0};
  HANDLE fenceEvent;
  ComPtr<D3D12MA::Allocator> allocator;
  ComPtr<ID3D12DescriptorHeap> globalSrvUavHeap;
  UINT srvUavDescriptorSize{0};
  uint32_t currentDescriptorOffset{0};
  static constexpr uint32_t MAX_DESCRIPTORS{1024}; // TODO experimental

  void createFactory();
  void enableDebugLayer(bool enableGPUValidation);
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandQueue();
  void createSyncObjects();
  void createAllocator();
  void createGlobalDescriptorHeap();
};
} // namespace elementalEngine::RHI