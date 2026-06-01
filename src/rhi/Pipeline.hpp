#pragma once

namespace elementalEngine::RHI {
class Pipeline {
public:
  virtual ~Pipeline() = default;

  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;

protected:
  Pipeline() = default;
};

} // namespace elementalEngine::RHI