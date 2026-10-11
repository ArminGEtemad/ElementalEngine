#pragma once

#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace elementalEngine::Renderer {
// handling the midpoint displacement pipeline. This handles both compute and
// graphics right now. before the calculation was happening using recursion on
// cpu. and then the information was sent to gpu. now everything is moved to
// gpu. the strikes are calculated with 4 threads

// TODO: in fire renderer as well there are duplicate of camera parameters that
// were built because at that moment it was faster to prototype and were
// forgotten to be fixed. CLEAN UP after gpu driven rendering
struct MidpointLightningParams {
  float viewProj[16];
  float cameraPos[4];
  float opacity;
  float thickness;
  float pad[2];
};

struct LightningStrike {
  float triggerTime; // when strike starts relative to the sequence launch
  float duration;    // total lifespan of an individual strike
  float peakOpacity;
  float thickness;
};

class LightningRenderer {
public:
  static constexpr uint32_t MAX_STRIKES = 4;
  static constexpr uint32_t MAX_SEGMENTS_PER_STRIKE = 2048;

  LightningRenderer(RHI::Device &device);
  ~LightningRenderer() = default;

  void triggerLightning(RHI::CommandList &cmdList, float targetX,
                        float targetZ);

  void update(float dt);

  void draw(RHI::CommandList &commandList, const float *viewProjMatrix,
            const glm::vec3 &cameraPos);

  // getter
  float getOpacity() const { return opacity; }

private:
  RHI::Device &device;

  std::unique_ptr<RHI::Pipeline> lightningPipeline;
  std::unique_ptr<RHI::Pipeline> computePipeline;
  std::vector<LightningStrike> strikes;
  std::unique_ptr<RHI::Buffer> segmentsBuffer;
  std::unique_ptr<RHI::Buffer> indirectDrawBuffer;

  // animation values
  float opacity{0.0f};
  float totDuration{0.0f};
  float timer{999.0f};
  uint32_t seedCounter{0};
  bool isFirstFrame{true};

  void createPipelines();
  void createBuffers();
};
} // namespace elementalEngine::Renderer