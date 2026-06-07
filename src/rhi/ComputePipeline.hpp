#pragma once

#include <cstdint>
namespace elementalEngine::RHI {
struct SimConfig {
  uint32_t gridWidth;
  uint32_t gridHeight;
  float dt;
  float pad_0;
};
class ComputePipeline {
public:
  virtual ~ComputePipeline() = default;

  ComputePipeline(const ComputePipeline &) = delete;
  ComputePipeline &operator=(const ComputePipeline &) = delete;

protected:
  ComputePipeline() = default;
};
} // namespace elementalEngine::RHI