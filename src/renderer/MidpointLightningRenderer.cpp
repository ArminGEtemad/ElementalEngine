#include "MidpointLightningRenderer.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "RHICommon.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace elementalEngine::Renderer {
LightningRenderer::LightningRenderer(RHI::Device &device) : device(device) {
  // initializing the randomizer
  std::random_device rd;
  randomizer.seed(rd());
  timer = totDuration;
  opacity = 0.0f;

  createLightningPipeline();
}

void LightningRenderer::generateJaggedPath(const V2 &startPoint,
                                           const V2 &endPoint, float displace,
                                           int generation, int maxGenerated,
                                           std::vector<V2> &outPoints) {
  // when reaching the max stop generating
  if (generation >= maxGenerated) {
    return;
  }

  // calculate the midpoint of a segment
  V2 mid = {(startPoint.x + endPoint.x) * 0.5f,
            (startPoint.y + endPoint.y) * 0.5f};

  // the midpoint displacement must be perpendicular to the startPoint -
  // endPoint vector
  // first calculate the perpendicular vector
  float dx = endPoint.x - startPoint.x;
  float dy = endPoint.y - startPoint.y;
  float lengthSeg = std::sqrt(dx * dx + dy * dy);

  // perpendicular is (-dy, dx) or (dy, -dx)
  V2 perpendicular = {0.0f, 0.0f};
  if (lengthSeg > 1e-5) {
    perpendicular.x = -dy / lengthSeg;
    perpendicular.y = dx / lengthSeg;
  }

  float midPointOffset = normalDist(randomizer) * displace;
  mid.x += perpendicular.x * midPointOffset;
  mid.y += perpendicular.y * midPointOffset;

  // do the same for the first half
  generateJaggedPath(startPoint, mid, displace * 0.5f, generation + 1,
                     maxGenerated, outPoints);
  outPoints.push_back(mid);

  // do the same for the second half
  generateJaggedPath(mid, endPoint, displace * 0.5f, generation + 1,
                     maxGenerated, outPoints);
}

void LightningRenderer::triggerLightning(float targetX, float targetY) {
  strikes.clear();
  timer = 0.0f;

  // Hardcoded the roof. TODO make it dynamic later
  V2 startPoint = {targetX, 800.0f};
  V2 endPoint = {targetX, targetY};

  // helper struct
  struct StrikeConfig {
    float triggerTime;
    float duration;
    float peakOpacity;
    float thickness;
    float displacement;
    int maxGenerations;
  };

  std::vector<StrikeConfig> configs = {{0.00f, 0.12f, 0.3f, 1.5f, 200.0f, 5},
                                       {0.12f, 0.10f, 0.5f, 1.5f, 200.0f, 5},
                                       {0.22f, 0.13f, 0.5f, 1.5f, 200.0f, 5},
                                       {0.35f, 0.45f, 1.0f, 5.5f, 150.0f, 6}};

  totDuration = 0.0f;

  for (const auto &config : configs) {
    LightningStrike strike;
    strike.triggerTime = config.triggerTime;
    strike.duration = config.duration;
    strike.peakOpacity = config.peakOpacity;
    strike.thickness = config.thickness;

    // every strike is unique
    std::vector<V2> points;
    points.push_back(startPoint);
    generateJaggedPath(startPoint, endPoint, config.displacement, 0,
                       config.maxGenerations, points);
    points.push_back(endPoint);
    strike.pointCount = static_cast<uint32_t>(points.size());

    // Allocate storage buffer and upload vertices
    size_t bufferSize = points.size() * sizeof(V2);
    strike.strikeBuffer = device.createBuffer(
        bufferSize, RHI::BufferUsage::Storage, RHI::MemoryProperty::CPUAccess);

    void *mappedData = strike.strikeBuffer->map();
    std::memcpy(mappedData, points.data(), bufferSize);
    strike.strikeBuffer->unmap();

    strikes.push_back(std::move(strike));

    // Keep track of the longest segment to know when the entire animation
    // finishes
    float endOfStrike = config.triggerTime + config.duration;
    if (endOfStrike > totDuration) {
      totDuration = endOfStrike;
    }
  }
}

void LightningRenderer::update(float dt) {
  if (timer < totDuration) {
    timer += dt;
    float t = timer;

    // Reset default opacity
    opacity = 0.0f;

    // Check which strike in our collection is currently active on the
    // timeline
    for (const auto &strike : strikes) {
      if (t >= strike.triggerTime && t < strike.triggerTime + strike.duration) {
        float strikeAge = t - strike.triggerTime;
        float progress = strikeAge / strike.duration;

        //  fast rise, linear decay
        if (progress < 0.20f) {
          opacity = (progress / 0.20f) * strike.peakOpacity;
        } else {
          opacity = (1.0f - (progress - 0.20f) / 0.80f) * strike.peakOpacity;
        }
        break;
      }
    }
  } else {
    opacity = 0.0f;
    timer = totDuration;
    // Free GPU buffers once the sequence completes
    strikes.clear();
  }
}

void LightningRenderer::createLightningPipeline() {
  using namespace RHI;

  PipelineConfig config;
  config.bindings = {
      {0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex}};

  config.pushConstants.size = sizeof(MidpointLightningParams);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Vertex | ShaderStage::Fragment;

  // We want standard additive blending so our glowing lightning arcs
  // brighten the background scene cleanly
  config.blendMode = Blendmode::Additive;

  lightningPipeline = device.createPipeline("midpoint_lightning_vs",
                                            "midpoint_lightning_fs", config);
}

void LightningRenderer::draw(RHI::CommandList &commandList,
                             const float *viewProjMatrix) {
  if (opacity <= 0.0f || strikes.empty()) {
    return;
  }

  // Iterate over our database to find and draw the currently active strike
  for (const auto &strike : strikes) {
    if (timer >= strike.triggerTime &&
        timer < strike.triggerTime + strike.duration) {
      if (strike.pointCount < 2)
        continue;

      commandList.bindPipeline(*lightningPipeline);

      MidpointLightningParams params{};
      std::memcpy(params.viewProj, viewProjMatrix, sizeof(float) * 16);
      params.opacity = opacity;
      params.thickness = strike.thickness;

      commandList.pushConstants(0, sizeof(MidpointLightningParams), &params,
                                RHI::ShaderStage::AllGraphics);
      commandList.bindStorageBuffer(0, strike.strikeBuffer.get());

      uint32_t vertexCount = (strike.pointCount - 1) * 6;
      commandList.draw(vertexCount, 1, 0, 0);
      break; // Only draw the active strike, then exit
    }
  }
}

} // namespace elementalEngine::Renderer