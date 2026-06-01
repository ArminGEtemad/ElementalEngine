#pragma once

namespace elementalEngine::RHI {
enum class GraphicsAPI {
  Vulkan,
  DirectX12,
};

struct DeviceConfig {
  bool enableValidationLayers = true;
  bool enableGPUAssistedValidatioLayer = false;
};

} // namespace elementalEngine::RHI