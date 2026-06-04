#pragma once

#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include "Swapchain.hpp"
#include "Window.hpp"
#include <memory>

namespace elementalEngine::RHI {

class Swapchain;
class CommandList;
class Device {
public:
  virtual ~Device() = default;
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;

  virtual std::unique_ptr<Swapchain>
  createSwapchain(WindowHandling &window) = 0;
  virtual std::unique_ptr<CommandList> createCommandList() = 0;
  virtual std::unique_ptr<Pipeline> createPipeline() = 0;
  virtual std::unique_ptr<Buffer> createBuffer(size_t size, BufferUsage usage,
                                               MemoryProperty memory) = 0;

  virtual GraphicsAPI getAPI() const = 0;
  virtual void waitIdle() = 0;
  virtual void submit(CommandList *commandList, Swapchain *swapchain) = 0;

protected:
  Device() = default;
};

class RHIFilter {
public:
  static std::unique_ptr<Device> createDevice(GraphicsAPI api,
                                              const DeviceConfig &config,
                                              WindowHandling &window);
};
} // namespace elementalEngine::RHI