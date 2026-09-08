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
    const glm::vec3 &startPoint, const glm::vec3 &endPoint, float displace,
    int generation, int maxGenerated, float scale,
    std::vector<Segments> &outSegments) {
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
  glm::vec3 mid = (startPoint + endPoint) * 0.5f;

  // the midpoint displacement must be perpendicular to the startPoint -
  // endPoint vector
  // first calculate the perpendicular vector
  glm::vec3 direction = endPoint - startPoint;
  float lengthSeg = glm::length(direction);

  if (lengthSeg > 1e-5f) {
    glm::vec3 mainDir = direction / lengthSeg;

    // generate a completely random vector and take the cross product for
    // perpendicular
    glm::vec3 randomVec =
        glm::normalize(glm::vec3(normalDist(randomizer), normalDist(randomizer),
                                 normalDist(randomizer)));

    // fallback if the random vector happens to be exactly parallel to the
    // direction
    if (glm::abs(glm::dot(mainDir, randomVec)) > 0.99f) {
      randomVec = glm::vec3(1.0f, 0.0f, 0.0f);
      if (glm::abs(glm::dot(mainDir, randomVec)) > 0.99f) {
        randomVec = glm::vec3(0.0f, 1.0f, 0.0f);
      }
    }
    glm::vec3 perpendicular = glm::normalize(glm::cross(mainDir, randomVec));

    float midPointOffset = normalDist(randomizer) * displace;
    mid += perpendicular * midPointOffset;
  }

  // forking logic and allow branching
  if (generation == 2 || generation == 3) {
    float forkRoll = (normalDist(randomizer) + 1.0f) * 0.5f;
    if (forkRoll < 0.50f) { // 50% chance to fork a sister branch
      // direction vector
      glm::vec3 tDir = endPoint - mid;
      float tLen = glm::length(tDir);

      if (tLen > 1.0f) {
        glm::vec3 mainDir = tDir / tLen;

        // choosing a random downward angle
        float angleSign = (normalDist(randomizer) > 0.0f) ? 1.0f : -1.0f;
        float angle = angleSign *
                      (20.0f + (normalDist(randomizer) + 1.0f) * 0.5f * 0.25f) *
                      3.1415f / 180.0f;

        glm::vec3 randomAxis = glm::normalize(
            glm::vec3(normalDist(randomizer), normalDist(randomizer),
                      normalDist(randomizer)));
        if (std::abs(glm::dot(mainDir, randomAxis)) > 0.99f)
          randomAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 rotAxis = glm::normalize(glm::cross(mainDir, randomAxis));

        // rotation matrix
        float cosAngle = std::cos(angle);
        float sinAngle = std::sin(angle);
        glm::vec3 branchDir =
            mainDir * cosAngle + glm::cross(rotAxis, mainDir) * sinAngle +
            rotAxis * glm::dot(rotAxis, mainDir) * (1.0f - cosAngle);

        glm::vec3 branchEnd = mid + branchDir * tLen;
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

void LightningRenderer::triggerLightning(float targetX, float targetZ) {
  strikes.clear();
  timer = 0.0f;

  // Hardcoded the roof. TODO make it dynamic later
  glm::vec3 startPoint = glm::vec3(targetX, 20.0f, targetZ);
  glm::vec3 endPoint = glm::vec3(targetX, 0.0f, targetZ);

  // helper struct
  struct StrikeConfig {
    float triggerTime;
    float duration;
    float peakOpacity;
    float thickness;
    float displacement;
    int maxGenerations;
  };

  std::vector<StrikeConfig> configs = {{0.00f, 0.06f, 0.3f, 0.05f, 1.8f, 8},
                                       {0.06f, 0.05f, 0.5f, 0.08f, 2.0f, 8},
                                       {0.11f, 0.07f, 0.5f, 0.08f, 2.0f, 8},
                                       {0.18f, 0.05f, 1.0f, 0.15f, 1.5f, 6}};

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
  config.depthState.depthTestEnable = true;
  config.depthState.depthWriteEnable = false;
  config.cullMode = CullMode::None;
  config.colorFormat = TextureFormat::B8G8R8A8_SRGB;
  config.depthFormat = TextureFormat::D32_FLOAT;
  config.hasDepthAttachment = true;

  lightningPipeline = device.createPipeline("midpoint_lightning_vs",
                                            "midpoint_lightning_fs", config);
}

void LightningRenderer::draw(RHI::CommandList &commandList,
                             const float *viewProjMatrix,
                             const glm::vec3 &cameraPos) {
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
      params.cameraPos[0] = cameraPos.x;
      params.cameraPos[1] = cameraPos.y;
      params.cameraPos[2] = cameraPos.z;
      params.cameraPos[3] = 1.0f;
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