#include "Device.hpp"
#include "RHICommon.hpp"
#include "dx12/DX12Device.hpp"
#include "vulkan/VulkanDevice.hpp"
#include <stdexcept>

namespace elementalEngine::RHI {
std::unique_ptr<Device> RHIFilter::createDevice(GraphicsAPI api,
                                                const DeviceConfig &config,
                                                WindowHandling &window) {
  switch (api) {
  case GraphicsAPI::Vulkan:
    return std::make_unique<VulkanDevice>(config, window);
  case GraphicsAPI::DirectX12:
    return std::make_unique<DX12Device>(config, window);
  default:
    throw std::runtime_error("Unsupported API selected!");
  }
}

} // namespace elementalEngine::RHI