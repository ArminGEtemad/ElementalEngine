#include "MidpointLightningRenderer.hpp"
#include <cstring>

namespace elementalEngine::Renderer {

struct StrikeComputeConfig {
  float displacement;
  uint32_t maxGenerations;
  float pad[2];
};

struct LightningComputeConstants {
  float startPoint[3];
  float pad0;
  float endPoint[3];
  uint32_t randomSeed;
  StrikeComputeConfig configs[4];
};

LightningRenderer::LightningRenderer(RHI::Device &device) : device(device) {
  timer = 999.0f;
  opacity = 0.0f;

  createBuffers();
  createPipelines();
}

void LightningRenderer::createBuffers() {
  using namespace RHI;

  // 4 strikes * 2048 max segments per strike * 32 bytes per segment
  uint32_t totalSegments = MAX_STRIKES * MAX_SEGMENTS_PER_STRIKE;
  segmentsBuffer = device.createBuffer(
      totalSegments * 32, BufferUsage::Storage | BufferUsage::Vertex,
      MemoryProperty::GPULocal);

  // Buffer to hold 4 Draw Commands (1 for each strike)
  indirectDrawBuffer = device.createBuffer(
      MAX_STRIKES * sizeof(DrawIndirectCommand),
      BufferUsage::Storage | BufferUsage::Indirect | BufferUsage::TransferDst,
      MemoryProperty::GPULocal);
}

void LightningRenderer::createPipelines() {
  using namespace RHI;

  PipelineConfig compConfig;
  compConfig.bindings = {
      {0, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute}, // segmentsBuffer
      {1, DescriptorType::StorageBuffer, 1,
       ShaderStage::Compute} // indirectDrawBuffer
  };
  compConfig.pushConstants.size = sizeof(LightningComputeConstants);
  compConfig.pushConstants.offset = 0;
  compConfig.pushConstants.stage = ShaderStage::Compute;

  computePipeline =
      device.createComputePipeline("lightning_compute", compConfig);

  PipelineConfig graphConfig;
  graphConfig.bindings = {
      {0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex}};
  graphConfig.pushConstants.size = sizeof(MidpointLightningParams);
  graphConfig.pushConstants.offset = 0;
  graphConfig.pushConstants.stage = ShaderStage::Vertex | ShaderStage::Fragment;

  graphConfig.blendMode = Blendmode::Additive;
  graphConfig.depthState.depthTestEnable = true;
  graphConfig.depthState.depthWriteEnable = false;
  graphConfig.cullMode = CullMode::None;
  graphConfig.colorFormat = TextureFormat::B8G8R8A8_SRGB;
  graphConfig.depthFormat = TextureFormat::D32_FLOAT;
  graphConfig.hasDepthAttachment = true;

  lightningPipeline = device.createPipeline(
      "midpoint_lightning_vs", "midpoint_lightning_fs", graphConfig);
}

void LightningRenderer::triggerLightning(RHI::CommandList &cmdList,
                                         float targetX, float targetZ) {
  strikes.clear();
  timer = 0.0f;
  totDuration = 0.0f;

  struct CPUSideStrikeConfig {
    float triggerTime;
    float duration;
    float peakOpacity;
    float thickness;
    float displacement;
    int maxGenerations;
  };

  std::vector<CPUSideStrikeConfig> strikesConfig = {
      {0.00f, 0.06f, 0.3f, 0.05f, 1.8f, 8},
      {0.06f, 0.05f, 0.5f, 0.08f, 2.0f, 8},
      {0.11f, 0.07f, 0.5f, 0.08f, 2.0f, 8},
      {0.18f, 0.05f, 1.0f, 0.15f, 1.5f, 6}};

  LightningComputeConstants pc{};
  pc.startPoint[0] = targetX;
  pc.startPoint[1] = 20.0f;
  pc.startPoint[2] = targetZ;
  pc.pad0 = 0.0f;
  pc.endPoint[0] = targetX;
  pc.endPoint[1] = 0.0f;
  pc.endPoint[2] = targetZ;

  seedCounter++;
  pc.randomSeed = seedCounter * 1337;

  for (size_t i = 0; i < MAX_STRIKES; i++) {
    LightningStrike s;
    s.triggerTime = strikesConfig[i].triggerTime;
    s.duration = strikesConfig[i].duration;
    s.peakOpacity = strikesConfig[i].peakOpacity;
    s.thickness = strikesConfig[i].thickness;
    strikes.push_back(s);

    if (s.triggerTime + s.duration > totDuration) {
      totDuration = s.triggerTime + s.duration;
    }

    pc.configs[i].displacement = strikesConfig[i].displacement;
    pc.configs[i].maxGenerations = strikesConfig[i].maxGenerations;
  }
  const float colComp[4] = {0.8f, 0.8f, 0.0f, 1.0f};
  cmdList.beginDebugMarker("Lightning Generate Compute", colComp);

  RHI::ResourceState indirectFrom = isFirstFrame
                                        ? RHI::ResourceState::Undefined
                                        : RHI::ResourceState::IndirectArgument;
  cmdList.transitionBuffer(indirectDrawBuffer.get(), indirectFrom,
                           RHI::ResourceState::TransferDst);
  cmdList.clearBuffer(indirectDrawBuffer.get(), 0);
  cmdList.transitionBuffer(indirectDrawBuffer.get(),
                           RHI::ResourceState::TransferDst,
                           RHI::ResourceState::UnorderedAccess);

  RHI::ResourceState segFrom = isFirstFrame
                                   ? RHI::ResourceState::Undefined
                                   : RHI::ResourceState::ShaderResource;
  cmdList.transitionBuffer(segmentsBuffer.get(), segFrom,
                           RHI::ResourceState::UnorderedAccess);

  isFirstFrame = false;

  cmdList.bindPipeline(*computePipeline);
  cmdList.bindStorageBuffer(0, segmentsBuffer.get());
  cmdList.bindStorageBuffer(1, indirectDrawBuffer.get());

  cmdList.pushConstants(0, sizeof(LightningComputeConstants), &pc,
                        RHI::ShaderStage::Compute);

  // 1 thread group with 4 threads. 1 thread for each strike
  cmdList.dispatch(1, 1, 1);

  // Transition to ShaderResource & IndirectArgument for the Graphics pass
  cmdList.transitionBuffer(segmentsBuffer.get(),
                           RHI::ResourceState::UnorderedAccess,
                           RHI::ResourceState::ShaderResource);
  cmdList.transitionBuffer(indirectDrawBuffer.get(),
                           RHI::ResourceState::UnorderedAccess,
                           RHI::ResourceState::IndirectArgument);

  cmdList.endDebugMarker();
}

void LightningRenderer::update(float dt) {
  if (timer < totDuration) {
    timer += dt;
    opacity = 0.0f;

    for (const auto &strike : strikes) {
      if (timer >= strike.triggerTime &&
          timer < strike.triggerTime + strike.duration) {
        float progress = (timer - strike.triggerTime) / strike.duration;
        opacity = (progress < 0.20f) ? (progress / 0.20f) * strike.peakOpacity
                                     : (1.0f - (progress - 0.20f) / 0.80f) *
                                           strike.peakOpacity;
        break;
      }
    }
  } else {
    opacity = 0.0f;
    timer = 999.0f;
  }
}

void LightningRenderer::draw(RHI::CommandList &cmdList,
                             const float *viewProjMatrix,
                             const glm::vec3 &cameraPos) {
  if (opacity <= 0.0f || strikes.empty())
    return;

  // marker color
  const float colorLightning[4] = {0.45f, 0.85f, 1.0f, 1.0f};
  cmdList.beginDebugMarker("Lightning Strike Render", colorLightning);

  cmdList.bindPipeline(*lightningPipeline);

  for (size_t i = 0; i < strikes.size(); i++) {
    const auto &strike = strikes[i];
    if (timer >= strike.triggerTime &&
        timer < strike.triggerTime + strike.duration) {

      MidpointLightningParams params{};
      std::memcpy(params.viewProj, viewProjMatrix, sizeof(float) * 16);
      params.cameraPos[0] = cameraPos.x;
      params.cameraPos[1] = cameraPos.y;
      params.cameraPos[2] = cameraPos.z;
      params.cameraPos[3] = 1.0f;
      params.opacity = opacity;
      params.thickness = strike.thickness;

      cmdList.pushConstants(0, sizeof(MidpointLightningParams), &params,
                            RHI::ShaderStage::AllGraphics);

      cmdList.bindStorageBuffer(0, segmentsBuffer.get());

      size_t indirectOffset = i * sizeof(RHI::DrawIndirectCommand);
      cmdList.drawIndirect(indirectDrawBuffer.get(), indirectOffset, 1,
                           sizeof(RHI::DrawIndirectCommand));

      break;
    }
  }
  cmdList.endDebugMarker();
}

} // namespace elementalEngine::Renderer