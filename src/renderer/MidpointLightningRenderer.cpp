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
  timer = duration;
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
  // Hardcoded the roof. TODO make it dynamic later
  V2 startPoint = {targetX, 800.0f};
  V2 endPoint = {targetX, targetY};

  // first bolt
  // recursive path generation
  std::vector<V2> points1;
  points1.push_back(startPoint);
  generateJaggedPath(startPoint, endPoint, 220.0f, 0, 5, points1);
  points1.push_back(endPoint);
  pointCount1 = static_cast<uint32_t>(points1.size());
  size_t bufferSize1 = points1.size() * sizeof(V2);
  lightningBuffer1 = device.createBuffer(bufferSize1, RHI::BufferUsage::Storage,
                                         RHI::MemoryProperty::CPUAccess);
  void *mappedData1 = lightningBuffer1->map();
  std::memcpy(mappedData1, points1.data(), bufferSize1);
  lightningBuffer1->unmap();

  // second bolt -> main powerful strike
  std::vector<V2> points2;
  points2.push_back(startPoint);
  generateJaggedPath(startPoint, endPoint, 120.0f, 0, 6, points2);
  points2.push_back(endPoint);
  pointCount2 = static_cast<uint32_t>(points2.size());
  size_t bufferSize2 = points2.size() * sizeof(V2);
  lightningBuffer2 = device.createBuffer(bufferSize2, RHI::BufferUsage::Storage,
                                         RHI::MemoryProperty::CPUAccess);
  void *mappedData2 = lightningBuffer2->map();
  std::memcpy(mappedData2, points2.data(), bufferSize2);
  lightningBuffer2->unmap();

  // Reset clock to run the two-bolt sequence
  timer = 0.0f;
  opacity = 0.0f;
}

void LightningRenderer::update(float dt) {
  if (timer < duration) {
    timer += dt;
    float t = timer;

    auto lerp = [](float a, float b, float f) { return a + f * (b - a); };

    // --- TWO-BOLT DISCHARGE TIMELINE ---
    if (t < 0.20f) {
      // first bolt that fades out quicker
      if (t < 0.08f) {
        opacity = lerp(0.0f, 0.45f, t / 0.08f); // raises to 45% brightness
      } else {
        opacity =
            lerp(0.45f, 0.0f,
                 (t - 0.08f) / 0.12f); // fades completely to dark at 0.20s
      }
    } else {
      // Return Strike "the main bolt with brighter visuals"
      float tReturn = t - 0.20f;
      if (tReturn < 0.06f) {
        opacity = lerp(0.0f, 1.0f, tReturn / 0.06f); // 100% brightness
      } else {
        opacity = lerp(1.0f, 0.0f,
                       (tReturn - 0.06f) / 0.49f); // Slow trailing fadeout
      }
    }
  } else {
    opacity = 0.0f;
    timer = duration;
    pointCount1 = 0;
    pointCount2 = 0;
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
  // no drawing if no lightning active
  if (opacity <= 0.0f) {
    return;
  }

  commandList.bindPipeline(*lightningPipeline);

  MidpointLightningParams params{};
  std::memcpy(params.viewProj, viewProjMatrix, sizeof(float) * 16);
  params.opacity = opacity;

  // timed buffer switch with dynamical thickness
  if (timer < 0.2f) {
    if (pointCount1 < 2)
      return;

    params.thickness = 2.0f;
    commandList.pushConstants(0, sizeof(MidpointLightningParams), &params,
                              RHI::ShaderStage::AllGraphics);
    // (Set 0, Binding 0)
    commandList.bindStorageBuffer(0, lightningBuffer1.get());
    // (6 vertices per segment),
    // (pointCount - 1) * 6 total vertices
    uint32_t vertexCount = (pointCount1 - 1) * 6;
    commandList.draw(vertexCount, 1, 0, 0);
  } else {
    if (pointCount2 < 2)
      return;

    params.thickness = 5.0f;
    commandList.pushConstants(0, sizeof(MidpointLightningParams), &params,
                              RHI::ShaderStage::AllGraphics);
    // (Set 0, Binding 0)
    commandList.bindStorageBuffer(0, lightningBuffer2.get());
    // (6 vertices per segment),
    // (pointCount - 1) * 6 total vertices
    uint32_t vertexCount = (pointCount2 - 1) * 6;
    commandList.draw(vertexCount, 1, 0, 0);
  }
}

} // namespace elementalEngine::Renderer