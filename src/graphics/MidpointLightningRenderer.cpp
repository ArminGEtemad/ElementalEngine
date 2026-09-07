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

void LightningRenderer::generateJaggedPaths(
    const V2 &startPoint, const V2 &endPoint, float displace, int generation,
    int maxGenerated, float scale, std::vector<Segments> &outSegments) {
  // when reaching the max stop generating
  if (generation >= maxGenerated) {
    Segments seg;
    seg.p0 = startPoint;
    seg.p1 = endPoint;
    seg.scale = scale;
    seg.pad0 = 0.0f;
    outSegments.push_back(seg);
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

  // forking logic and allow branching
  if (generation == 2 || generation == 3) {
    float forkRoll = (normalDist(randomizer) + 1.0f) * 0.5f;
    if (forkRoll < 0.50f) { // 50% chance to fork a sister branch
      // direction vector
      float tDx = endPoint.x - mid.x;
      float tDy = endPoint.y - mid.y;
      float tLen = std::sqrt(tDx * tDx + tDy * tDy);

      if (tLen > 1.0f) {
        V2 mainDir = {tDx / tLen, tDy / tLen};

        // choosing a random downward angle
        float angleSign = (normalDist(randomizer) > 0.0f) ? 1.0f : -1.0f;
        float angle = angleSign *
                      (20.0f + (normalDist(randomizer) + 1.0f) * 0.5f * 0.25f) *
                      3.1415f / 180.0f;

        // rotation matrix
        float cosAngle = std::cos(angle);
        float sinAngle = std::sin(angle);
        V2 branchDir;
        branchDir.x = mainDir.x * cosAngle - mainDir.y * sinAngle;
        branchDir.y = mainDir.x * sinAngle + mainDir.y * cosAngle;

        V2 branchEnd;
        branchEnd.x = mid.x + branchDir.x * tLen;
        branchEnd.y = mid.y + branchDir.y * tLen;
        // Recursively generate segments for the branch (dimmer scale, smaller
        // displacement)
        generateJaggedPaths(mid, branchEnd, displace * 0.5f, generation + 1,
                            maxGenerated, scale * 0.35f, outSegments);
      }
    }
  }

  // Generate main trunk segments recursively
  generateJaggedPaths(startPoint, mid, displace * 0.5f, generation + 1,
                      maxGenerated, scale, outSegments);
  generateJaggedPaths(mid, endPoint, displace * 0.5f, generation + 1,
                      maxGenerated, scale, outSegments);
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

  std::vector<StrikeConfig> configs = {{0.00f, 0.12f, 0.3f, 1.5f, 180.0f, 8},
                                       {0.12f, 0.10f, 0.5f, 2.5f, 200.0f, 8},
                                       {0.22f, 0.13f, 0.5f, 2.5f, 200.0f, 8},
                                       {0.55f, 0.45f, 1.0f, 5.5f, 150.0f, 6}};

  totDuration = 0.0f;

  for (const auto &config : configs) {
    LightningStrike strike;
    strike.triggerTime = config.triggerTime;
    strike.duration = config.duration;
    strike.peakOpacity = config.peakOpacity;
    strike.thickness = config.thickness;

    // every strike is unique
    std::vector<Segments> segments;
    generateJaggedPaths(startPoint, endPoint, config.displacement, 0,
                        config.maxGenerations, 1.0f, segments);
    strike.segmentCount = static_cast<uint32_t>(segments.size());

    // Allocate storage buffer and upload vertices
    size_t bufferSize = segments.size() * sizeof(Segments);
    strike.strikeBuffer = device.createBuffer(
        bufferSize, RHI::BufferUsage::Storage, RHI::MemoryProperty::CPUAccess);

    void *mappedData = strike.strikeBuffer->map();
    std::memcpy(mappedData, segments.data(), bufferSize);
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
      if (strike.segmentCount == 0)
        continue;

      commandList.bindPipeline(*lightningPipeline);

      MidpointLightningParams params{};
      std::memcpy(params.viewProj, viewProjMatrix, sizeof(float) * 16);
      params.opacity = opacity;
      params.thickness = strike.thickness;

      commandList.pushConstants(0, sizeof(MidpointLightningParams), &params,
                                RHI::ShaderStage::AllGraphics);
      commandList.bindStorageBuffer(0, strike.strikeBuffer.get());

      uint32_t vertexCount = strike.segmentCount * 6;
      commandList.draw(vertexCount, 1, 0, 0);
      break; // Only draw the active strike, then exit
    }
  }
}

} // namespace elementalEngine::Renderer