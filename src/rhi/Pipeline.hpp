#pragma once

#include "RHICommon.hpp"
namespace elementalEngine::RHI {
struct SimConfig {
  uint32_t gridWidth;
  uint32_t gridHeight;
  float dt;
  float forceY;
};
class Pipeline {
public:
  virtual ~Pipeline() = default;

  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;

  // unifying compute and graphics pipeline
  virtual PipelineBindPoint getBindPoint() const = 0;

protected:
  Pipeline() = default;
};

} // namespace elementalEngine::RHI