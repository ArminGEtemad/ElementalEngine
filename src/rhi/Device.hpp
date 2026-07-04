#pragma once

#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include "Texture.hpp"
#include "Window.hpp"
#include <cstdint>
#include <memory>

namespace elementalEngine::RHI {

class Swapchain;
class CommandList;
class Pipeline;
class ComputePipeline;
class Buffer;
class Texture;
class Device {
public:
  virtual ~Device() = default;
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;

  virtual std::unique_ptr<Swapchain>
  createSwapchain(WindowHandling &window) = 0;
  virtual std::unique_ptr<CommandList> createCommandList() = 0;

  virtual std::unique_ptr<Pipeline>
  createPipeline(const std::string &vertexShaderName,
                 const std::string &fragmentShaderName,
                 const PipelineConfig &config) = 0;

  virtual std::unique_ptr<Pipeline>
  createComputePipeline(const std::string &computeShaderName,
                        const PipelineConfig &config) = 0;

  virtual std::unique_ptr<Buffer> createBuffer(size_t size, BufferUsage usage,
                                               MemoryProperty memory) = 0;
  virtual std::unique_ptr<Texture> createTexture(uint32_t gridWidth,
                                                 uint32_t gridHeight,
                                                 TextureFormat format,
                                                 TextureUsage usage) = 0;

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