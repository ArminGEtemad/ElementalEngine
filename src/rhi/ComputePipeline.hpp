#pragma once

namespace elementalEngine::RHI {
class ComputePipeline {
public:
  virtual ~ComputePipeline() = default;

  ComputePipeline(const ComputePipeline &) = delete;
  ComputePipeline &operator=(const ComputePipeline &) = delete;

protected:
  ComputePipeline() = default;
};
} // namespace elementalEngine::RHI