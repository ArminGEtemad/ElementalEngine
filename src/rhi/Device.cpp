#include "Device.hpp"
#include "RHICommon.hpp"
#ifdef RHI_VULKAN_ENABLED
#include "vulkan/VulkanDevice.hpp"
#endif

#ifdef RHI_DX12_ENABLED
#include "dx12/DX12Device.hpp"
#endif
#include <stdexcept>

namespace elementalEngine::RHI {
std::unique_ptr<Device> RHIFilter::createDevice(GraphicsAPI api,
                                                const DeviceConfig &config,
                                                WindowHandling &window) {
  switch (api) {
#ifdef RHI_VULKAN_ENABLED
  case GraphicsAPI::Vulkan:
    return std::make_unique<VulkanDevice>(config, window);
#endif
#ifdef RHI_DX12_ENABLED
  case GraphicsAPI::DirectX12:
    return std::make_unique<DX12Device>(config, window);
#endif
  default:
    throw std::runtime_error("Unsupported API selected!");
  }
}

} // namespace elementalEngine::RHI