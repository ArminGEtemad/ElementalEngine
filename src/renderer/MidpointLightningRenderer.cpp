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
  mid.x += perpendicular.x + midPointOffset;
  mid.y += perpendicular.y + midPointOffset;

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

  std::vector<V2> points;
  points.push_back(startPoint);

  // recursive path generation
  generateJaggedPath(startPoint, endPoint, 200.0f, 0, 6, points);
  // get the end point
  points.push_back(endPoint);
  pointCount = static_cast<uint32_t>(points.size());
  opacity = 1.0f;

  size_t bufferSize = points.size() * sizeof(V2);
  lightningBuffer = device.createBuffer(bufferSize, RHI::BufferUsage::Storage,
                                        RHI::MemoryProperty::CPUAccess);

  void *mappedData = lightningBuffer->map();
  std::memcpy(mappedData, points.data(), bufferSize);
  lightningBuffer->unmap();
}

void LightningRenderer::update(float dt) {
  if (opacity > 0.0f) {
    opacity -= dt * fadeSpeed;
    if (opacity < 0.0f) {
      opacity = 0.0f;
    }
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
  if (pointCount < 2 || opacity <= 0.0f) {
    return;
  }

  commandList.bindPipeline(*lightningPipeline);

  MidpointLightningParams params{};
  std::memcpy(params.viewProj, viewProjMatrix, sizeof(float) * 16);
  params.opacity = opacity;
  commandList.pushConstants(0, sizeof(MidpointLightningParams), &params,
                            RHI::ShaderStage::AllGraphics);

  // (Set 0, Binding 0)
  commandList.bindStorageBuffer(0, lightningBuffer.get());

  // (6 vertices per segment),
  // (pointCount - 1) * 6 total vertices
  uint32_t vertexCount = (pointCount - 1) * 6;
  commandList.draw(vertexCount, 1, 0, 0);
}

} // namespace elementalEngine::Renderer