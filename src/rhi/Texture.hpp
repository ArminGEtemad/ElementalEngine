#pragma once

#include <cstdint>

namespace elementalEngine::RHI {

enum class TextureFormat {
  B8G8R8A8_SRGB,
  R8G8B8A8_UNORM,
  R16_FLOAT,
  R32_FLOAT,
  R32G32_FLOAT,
  R32G32B32A32_FLOAT,
  D32_FLOAT,
  D24_UNORM_S8_UINT
};

enum class TextureUsage {
  None = 0,
  ShaderResource = 1 << 0,         // sampled in shader
  UnorderedAccess = 1 << 1,        // Read/ write in compute
  RenderTarget = 1 << 2,           // rendered directly
  DepthStencilAttachment = 1 << 3, // depth and stencil buffer
  TransferSrc = 1 << 4,            // copy source
  TransferDst = 1 << 5,            // copy destination
  Storage = 1 << 6
};

// combining usage
inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
  return static_cast<TextureUsage>(static_cast<uint32_t>(a) |
                                   static_cast<uint32_t>(b));
};

inline bool operator&(TextureUsage a, TextureUsage b) {
  return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

class Texture {
public:
  virtual ~Texture() = default;

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  virtual uint32_t getWidth() const = 0;
  virtual uint32_t getHeight() const = 0;
  virtual uint32_t getDepth() const = 0;
  virtual TextureFormat getFormat() const = 0;
  virtual TextureUsage getUsage() const = 0;

protected:
  Texture() = default;
};

} // namespace elementalEngine::RHI