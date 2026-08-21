#include "SSFRRenderer.hpp"

namespace elementalEngine::Renderer {

SSFRRenderer::SSFRRenderer(RHI::Device &device, uint32_t width, uint32_t height)
    : device(device), currentWidth(width), currentHeight(height) {
  createRenderTargets(width, height);
  createPipelines();
}

void SSFRRenderer::onResize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0)
    return;
  currentWidth = width;
  currentHeight = height;
  createRenderTargets(width, height);
}

void SSFRRenderer::createRenderTargets(uint32_t width, uint32_t height) {
  using namespace RHI;

  fluidDepthTexture = device.createTexture(
      width, height, TextureFormat::R32_FLOAT,
      TextureUsage::RenderTarget | TextureUsage::ShaderResource);

  fluidThicknessTexture = device.createTexture(
      width, height, TextureFormat::R16_FLOAT,
      TextureUsage::RenderTarget | TextureUsage::ShaderResource);

  internalDepthBuffer =
      device.createTexture(width, height, TextureFormat::D32_FLOAT,
                           TextureUsage::DepthStencilAttachment);

  tempDepthTexture = device.createTexture(
      width, height, TextureFormat::R32_FLOAT,
      TextureUsage::ShaderResource | TextureUsage::Storage);

  blurredDepthTexture = device.createTexture(
      width, height, TextureFormat::R32_FLOAT,
      TextureUsage::ShaderResource | TextureUsage::Storage);
}

void SSFRRenderer::createPipelines() {
  using namespace RHI;

  PipelineConfig config{};
  config.bindings = {
      {0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex}};
  config.pushConstants.size = sizeof(SSFRPushConstants);
  config.pushConstants.offset = 0;
  config.pushConstants.stage = ShaderStage::Vertex | ShaderStage::Fragment;

  config.cullMode = CullMode::None;
  config.depthFormat = TextureFormat::D32_FLOAT;
  config.hasDepthAttachment = true;

  // Depth Pipeline
  config.colorFormat = TextureFormat::R32_FLOAT;
  config.depthState.depthTestEnable = true;
  config.depthState.depthWriteEnable = true;
  config.depthState.depthCompareOp = CompareOp::Less;
  config.blendMode = Blendmode::None;
  depthPipeline =
      device.createPipeline("SSFR_fluid_vs", "SSFR_depth_fs", config);

  // Thickness Pipeline
  config.colorFormat = TextureFormat::R16_FLOAT;
  config.depthState.depthWriteEnable =
      false; // Additive volume doesn't block pixels
  config.depthState.depthTestEnable = false;
  config.blendMode = Blendmode::Additive; // thickness gets accumulated
  thicknessPipeline =
      device.createPipeline("SSFR_fluid_vs", "SSFR_thickness_fs", config);

  // compute blur pipeline
  PipelineConfig blurConfig{};
  blurConfig.bindings = {
      {0, DescriptorType::SampledImage, 1,
       ShaderStage::Compute}, // Input Depth (t0)
      {1, DescriptorType::StorageImage, 1,
       ShaderStage::Compute} // Output Depth (u1)
  };

  struct BlurPushConstants {
    int blurDirX;
    int blurDirY;
    float filterRadius;
    float spatialScale;
    float depthScale;
    float pad[3];
  };
  blurConfig.pushConstants.size = sizeof(BlurPushConstants);
  blurConfig.pushConstants.offset = 0;
  blurConfig.pushConstants.stage = ShaderStage::Compute;

  blurPipeline = device.createComputePipeline("SSFR_depth_blur", blurConfig);

  PipelineConfig compConfig{};
  compConfig.bindings = {
      {0, DescriptorType::SampledImage, 1, ShaderStage::Fragment},
      {1, DescriptorType::SampledImage, 1, ShaderStage::Fragment},
      {2, DescriptorType::Sampler, 1, ShaderStage::Fragment}};

  struct CompPushConstants {
    float invProj[16];
    float invView[16];
    float proj[16];
    float lightDir[4];
  };
  compConfig.pushConstants.size = sizeof(CompPushConstants);
  compConfig.pushConstants.offset = 0;
  compConfig.pushConstants.stage = ShaderStage::Fragment;

  // draw OVER the Terrain
  compConfig.cullMode = CullMode::None;
  compConfig.colorFormat =
      TextureFormat::B8G8R8A8_SRGB; // Ensure this matches your swapchain
                                    // format!
  compConfig.depthFormat = TextureFormat::D32_FLOAT;
  compConfig.hasDepthAttachment = true;
  compConfig.depthState.depthTestEnable = true; // Occlude against terrain
  compConfig.depthState.depthWriteEnable =
      false; // Translucent things shouldn't write to depth
  compConfig.depthState.depthCompareOp = CompareOp::Less;
  compConfig.blendMode = Blendmode::Alpha;

  compositePipeline = device.createPipeline("SSFR_composite_vs",
                                            "SSFR_composite_fs", compConfig);
}

void SSFRRenderer::renderGBuffer(RHI::CommandList &cmdList,
                                 const Physics::PBFSlime &slimeSim,
                                 const float *viewMatrix,
                                 const float *projMatrix, float particleRadius,
                                 float worldSizeX, float worldSizeZ) {

  cmdList.transitionTexture(fluidDepthTexture.get(),
                            RHI::ResourceState::Undefined,
                            RHI::ResourceState::RenderTarget);
  cmdList.transitionTexture(fluidThicknessTexture.get(),
                            RHI::ResourceState::Undefined,
                            RHI::ResourceState::RenderTarget);
  cmdList.transitionTexture(internalDepthBuffer.get(),
                            RHI::ResourceState::Undefined,
                            RHI::ResourceState::DepthStencilWrite);

  SSFRPushConstants pc{};
  std::memcpy(pc.viewMatrix, viewMatrix, sizeof(float) * 16);
  std::memcpy(pc.projMatrix, projMatrix, sizeof(float) * 16);
  pc.domainWidth = 2000.0f;
  pc.domainDepth = 2000.0f;
  pc.worldSizeX = worldSizeX;
  pc.worldSizeZ = worldSizeZ;
  pc.particleRadius = particleRadius;

  // pass 1 depth
  RHI::RenderingInfo depthInfo{};
  depthInfo.renderWidth = currentWidth;
  depthInfo.renderHeight = currentHeight;

  RHI::RenderPassAttachment depthColorAtt{};
  depthColorAtt.texture = fluidDepthTexture.get();
  depthColorAtt.clear = true;
  depthColorAtt.clearColor[0] = -10000.0f; // Initialize with a far-away depth
  depthInfo.colorAttachments.push_back(depthColorAtt);

  RHI::DepthAttachment zAtt{};
  zAtt.texture = internalDepthBuffer.get();
  zAtt.clear = true;
  zAtt.clearDepth = 1.0f;
  zAtt.clearStencil = 0;
  depthInfo.depthAttachment = zAtt;

  cmdList.beginRendering(depthInfo);
  cmdList.setViewport(0.0f, 0.0f, (float)currentWidth, (float)currentHeight);
  cmdList.setScissor(0, 0, currentWidth, currentHeight);

  cmdList.bindPipeline(*depthPipeline);
  cmdList.bindStorageBuffer(0, slimeSim.getParticleBuffer());
  cmdList.pushConstants(0, sizeof(SSFRPushConstants), &pc,
                        RHI::ShaderStage::Vertex | RHI::ShaderStage::Fragment);
  cmdList.draw(6, slimeSim.getParticleCount(), 0, 0);
  cmdList.endRendering();

  // pass 2 thickness
  RHI::RenderingInfo thickInfo{};
  thickInfo.renderWidth = currentWidth;
  thickInfo.renderHeight = currentHeight;

  RHI::RenderPassAttachment thickColorAtt{};
  thickColorAtt.texture = fluidThicknessTexture.get();
  thickColorAtt.clear = true;
  thickColorAtt.clearColor[0] = 0.0f;
  thickInfo.colorAttachments.push_back(thickColorAtt);

  RHI::DepthAttachment thickZAtt = zAtt;
  thickZAtt.clear = false; // Re-use the generated Z-buffer from Pass 1 so
                           //  thickness respects overlaps!
  thickInfo.depthAttachment = thickZAtt;

  cmdList.beginRendering(thickInfo);
  cmdList.setViewport(0.0f, 0.0f, (float)currentWidth, (float)currentHeight);
  cmdList.setScissor(0, 0, currentWidth, currentHeight);

  cmdList.bindPipeline(*thicknessPipeline);
  cmdList.bindStorageBuffer(0, slimeSim.getParticleBuffer());
  cmdList.pushConstants(0, sizeof(SSFRPushConstants), &pc,
                        RHI::ShaderStage::Vertex | RHI::ShaderStage::Fragment);
  cmdList.draw(6, slimeSim.getParticleCount(), 0, 0);
  cmdList.endRendering();

  // Transition to ShaderResource for the next bluring
  cmdList.transitionTexture(fluidDepthTexture.get(),
                            RHI::ResourceState::RenderTarget,
                            RHI::ResourceState::ShaderResource);
  cmdList.transitionTexture(fluidThicknessTexture.get(),
                            RHI::ResourceState::RenderTarget,
                            RHI::ResourceState::ShaderResource);
}

void SSFRRenderer::renderBlur(RHI::CommandList &cmdList) {
  struct BlurPushConstants {
    int blurDirX;
    int blurDirY;
    float filterRadius;
    float spatialScale;
    float depthScale;
    float pad[3];
  } pc;

  pc.filterRadius = 20.0f; // Increase to melt the spheres more!
  pc.spatialScale = 5.0f;
  pc.depthScale = 0.5f; // Low value preserves sharp depth silhouettes

  uint32_t groupX = (currentWidth + 15) / 16;
  uint32_t groupY = (currentHeight + 15) / 16;

  cmdList.bindPipeline(*blurPipeline);

  // pass 1: HORIZONTAL BLUR (Raw Depth -> Temp)
  cmdList.transitionTexture(tempDepthTexture.get(),
                            RHI::ResourceState::Undefined,
                            RHI::ResourceState::UnorderedAccess);

  cmdList.bindTexture(0, fluidDepthTexture.get());     // Read raw depth
  cmdList.bindStorageImage(1, tempDepthTexture.get()); // Write to temp

  pc.blurDirX = 1;
  pc.blurDirY = 0;
  cmdList.pushConstants(0, sizeof(BlurPushConstants), &pc,
                        RHI::ShaderStage::Compute);
  cmdList.dispatch(groupX, groupY, 1);

  // pass 2: VERTICAL BLUR (Temp -> Blurred Depth)
  // Transition Temp to be Read, and Blurred to be Written
  cmdList.transitionTexture(tempDepthTexture.get(),
                            RHI::ResourceState::UnorderedAccess,
                            RHI::ResourceState::ShaderResource);
  cmdList.transitionTexture(blurredDepthTexture.get(),
                            RHI::ResourceState::Undefined,
                            RHI::ResourceState::UnorderedAccess);

  cmdList.bindTexture(0, tempDepthTexture.get());         // Read temp
  cmdList.bindStorageImage(1, blurredDepthTexture.get()); // Write to blurred

  pc.blurDirX = 0;
  pc.blurDirY = 1;
  cmdList.pushConstants(0, sizeof(BlurPushConstants), &pc,
                        RHI::ShaderStage::Compute);
  cmdList.dispatch(groupX, groupY, 1);

  // Final Transition
  cmdList.transitionTexture(blurredDepthTexture.get(),
                            RHI::ResourceState::UnorderedAccess,
                            RHI::ResourceState::ShaderResource);
}

void SSFRRenderer::renderComposite(RHI::CommandList &cmdList,
                                   RHI::Texture *swapchainTexture,
                                   RHI::Texture *terrainDepthTexture,
                                   const float *invViewMatrix,
                                   const float *invProjMatrix,
                                   const float *projMatrix,
                                   const float *lightDir) {

  RHI::RenderingInfo info{};
  info.renderWidth = currentWidth;
  info.renderHeight = currentHeight;

  RHI::RenderPassAttachment colorAtt{};
  colorAtt.texture = swapchainTexture;
  colorAtt.clear = false;
  info.colorAttachments.push_back(colorAtt);

  RHI::DepthAttachment depthAtt{};
  depthAtt.texture = terrainDepthTexture; // Bind the terrain's depth buffer
  depthAtt.clear = false;
  info.depthAttachment = depthAtt;

  cmdList.beginRendering(info);
  cmdList.setViewport(0.0f, 0.0f, (float)currentWidth, (float)currentHeight);
  cmdList.setScissor(0, 0, currentWidth, currentHeight);

  cmdList.bindPipeline(*compositePipeline);

  cmdList.bindTexture(0, blurredDepthTexture.get());
  cmdList.bindTexture(1, fluidThicknessTexture.get());
  cmdList.bindSampler(2);

  struct CompPushConstants {
    float invProj[16];
    float invView[16];
    float proj[16];
    float lightDir[4];
  } pc;

  std::memcpy(pc.invProj, invProjMatrix, sizeof(float) * 16);
  std::memcpy(pc.invView, invViewMatrix, sizeof(float) * 16);
  std::memcpy(pc.proj, projMatrix, sizeof(float) * 16);
  pc.lightDir[0] = lightDir[0];
  pc.lightDir[1] = lightDir[1];
  pc.lightDir[2] = lightDir[2];
  pc.lightDir[3] = 0.0f;

  cmdList.pushConstants(0, sizeof(CompPushConstants), &pc,
                        RHI::ShaderStage::Fragment);

  // Draw Full Screen Quad (3 vertices)
  cmdList.draw(3, 1, 0, 0);
  cmdList.endRendering();
}

} // namespace elementalEngine::Renderer