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

struct Segments {
  V2 p0;
  V2 p1;
  float scale;
  float pad0;
};

struct LightningStrike {
  float triggerTime; // when strike starts relative to the sequence launch
  float duration;    // total lifespan of an individual strike
  float peakOpacity;
  float thickness;
  std::unique_ptr<RHI::Buffer> strikeBuffer;
  uint32_t segmentCount = 0; // number of segments in a path
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

  std::unique_ptr<RHI::Pipeline> lightningPipeline;
  std::vector<LightningStrike> strikes;

  // animation values
  float opacity{0.0f};
  float totDuration{0.0f};
  float timer{999.0f};

  // random jagged displacement
  std::default_random_engine randomizer;
  std::uniform_real_distribution<float> normalDist{-1.0f, 1.0f};

  void createLightningPipeline();

  void generateJaggedPaths(const V2 &startPoint, const V2 &endPoint,
                           float displace, int generation, int maxGenerated,
                           float scale, std::vector<Segments> &outSegments);
};
} // namespace elementalEngine::Renderer