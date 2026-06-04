#pragma once

#include "RHICommon.hpp"
#include <cstddef>

namespace elementalEngine::RHI {
class Buffer {
public:
  virtual ~Buffer() = default;

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  virtual size_t getSize() const = 0;
  virtual BufferUsage getBufferUsage() const = 0;
  virtual MemoryProperty getMemoryProperty() const = 0;

  // Returns a pointer to the mapped memory block
  virtual void *map() = 0;

  // Invalidates the pointer and flushes writes to the GPU
  virtual void unmap() = 0;

protected:
  Buffer() = default;
};
} // namespace elementalEngine::RHI