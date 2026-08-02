#include "Device.hpp"
#include "RHICommon.hpp"
#include "vulkan/VulkanDevice.hpp"

namespace elementalEngine::RHI {
std::unique_ptr<Device> RHIFilter::createDevice(const DeviceConfig &config,
                                                WindowHandling &window) {
  // there is only one API right now. When DX or Metal is added the api toggle
  // can be added here
  return std::make_unique<VulkanDevice>(config, window);
}

} // namespace elementalEngine::RHI