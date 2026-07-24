#pragma once

#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include <cstdint>
#include <memory>
#include <random>
#include <vector>
namespace elementalEngine::Renderer {

// a 2D vector struct
struct V2 {
  float x, y;
};

// push consts
struct MidpointLightningParams {
  float viewProj[16];
  float opacity;
  float thickness;
  float pad[2];
};

class LightningRenderer {
public:
  LightningRenderer(RHI::Device &device);
  ~LightningRenderer() = default;

  // triggering a lightning strike
  void triggerLightning(float targetX, float targetY);

  // update the fade-out part
  void update(float dt);

  void draw(RHI::CommandList &commandList, const float *viewProjMatrix);

private:
  RHI::Device &device;

  // storage Buffer holds the lightning vertices
  // the number of lightnings are hardcoded for now maybe make it scalable later
  std::unique_ptr<RHI::Buffer> lightningBuffer1;
  std::unique_ptr<RHI::Buffer> lightningBuffer2;

  uint32_t pointCount1{0};
  uint32_t pointCount2{0};

  std::unique_ptr<RHI::Pipeline> lightningPipeline;

  // animation values
  float opacity = 0.0f;
  const float duration = 0.75f; // fades out completely after 2 strokes
  float timer = 999.0f;

  // random jagged displacement
  std::default_random_engine randomizer;
  std::uniform_real_distribution<float> normalDist{-1.0f, 1.0f};

  void createLightningPipeline();

  void generateJaggedPath(const V2 &startPoint, const V2 &endPoint,
                          float displace, int generation, int maxGenerated,
                          std::vector<V2> &outPoints);
};
} // namespace elementalEngine::Renderer