#include "TerrainPass.hpp"
#include "TerrainMesh.hpp"
#include <cstring>

namespace elementalEngine::Graphics {

TerrainPass::TerrainPass(RHI::Device &device, RHI::Swapchain &swapchain)
    : device(device), camera(60.0f,
                             static_cast<float>(swapchain.getWidth()) /
                                 static_cast<float>(swapchain.getHeight()),
                             0.1f, 1000.0f) {

  // Set initial position backed up and elevated
  glm::vec3 startPos{15.0f, 30.0f, 15.0f};
  camera.setPosition(startPos);

  // Orient camera to look directly at the origin (0, 0, 0)
  glm::vec3 direction = glm::normalize(glm::vec3(0.0f) - startPos);
  glm::quat initialRotation =
      glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
  camera.setOrientation(initialRotation);

  createDepthTarget(swapchain.getWidth(), swapchain.getHeight());
  createBuffers();
  createPipeline();
}

void TerrainPass::createDepthTarget(uint32_t width, uint32_t height) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    // create one for each frame
    depthTextures[i] =
        device.createTexture(width, height, RHI::TextureFormat::D32_FLOAT,
                             RHI::TextureUsage::DepthStencilAttachment |
                                 RHI::TextureUsage::ShaderResource);
  }
}

void TerrainPass::createBuffers() {
  // Generate 64x64 terrain grid covering 20x20 world units
  TerrainMesh terrain = TerrainMesh::generateGrid(64, 64, 20.0f, 20.0f);

  size_t vertexBufferSize = terrain.Vertices.size() * sizeof(RHI::Vertex3D);
  vertexBuffer = device.createBuffer(
      vertexBufferSize,
      RHI::BufferUsage::Storage | RHI::BufferUsage::TransferDst,
      RHI::MemoryProperty::GPULocal);
  device.uploadBufferData(vertexBuffer.get(), terrain.Vertices.data(),
                          vertexBufferSize);

  size_t indexBufferSize = terrain.Indices.size() * sizeof(uint32_t);
  indexBuffer = device.createBuffer(
      indexBufferSize, RHI::BufferUsage::Index | RHI::BufferUsage::TransferDst,
      RHI::MemoryProperty::GPULocal);
  device.uploadBufferData(indexBuffer.get(), terrain.Indices.data(),
                          indexBufferSize);

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    cameraUniformBuffers[i] = device.createBuffer(
        sizeof(Core::CameraFrameData), RHI::BufferUsage::Uniform,
        RHI::MemoryProperty::CPUAccess);
  }

  totalChunks = static_cast<uint32_t>(terrain.Chunks.size());

  size_t chunkBufferSize = totalChunks * sizeof(TerrainChunkData);
  chunkDataBuffer = device.createBuffer(
      chunkBufferSize,
      RHI::BufferUsage::Storage | RHI::BufferUsage::TransferDst,
      RHI::MemoryProperty::GPULocal);
  device.uploadBufferData(chunkDataBuffer.get(), terrain.Chunks.data(),
                          chunkBufferSize);

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    indirectDrawBuffers[i] = device.createBuffer(
        totalChunks * sizeof(RHI::DrawIndexedIndirectCommand),
        RHI::BufferUsage::Storage | RHI::BufferUsage::Indirect |
            RHI::BufferUsage::TransferDst,
        RHI::MemoryProperty::GPULocal);
  }
}

void TerrainPass::createPipeline() {
  RHI::PipelineConfig cullConfig{};
  cullConfig.bindings = {
      {0, RHI::DescriptorType::StorageBuffer, 1,
       RHI::ShaderStage::Compute}, // chunkData
      {1, RHI::DescriptorType::UniformBuffer, 1,
       RHI::ShaderStage::Compute}, // camera UBO
      {2, RHI::DescriptorType::StorageBuffer, 1,
       RHI::ShaderStage::Compute} // indirect buffer
  };
  cullConfig.pushConstants.size = sizeof(uint32_t); // totalChunks
  cullConfig.pushConstants.offset = 0;
  cullConfig.pushConstants.stage = RHI::ShaderStage::Compute;
  cullPipeline =
      device.createComputePipeline("terrain_cull_compute", cullConfig);

  RHI::PipelineConfig config{};

  // Binding 0: StorageBuffer for Vertex Pulling (t type)
  config.bindings.push_back({.bindingSlot = 0,
                             .type = RHI::DescriptorType::StorageBuffer,
                             .count = 1,
                             .stage = RHI::ShaderStage::Vertex});

  // Binding 1: UniformBuffer for Camera FrameData (b type)
  config.bindings.push_back({.bindingSlot = 1,
                             .type = RHI::DescriptorType::UniformBuffer,
                             .count = 1,
                             .stage = RHI::ShaderStage::Vertex});

  // 3D Depth & Rasterizer
  config.cullMode = RHI::CullMode::None; // 2D shape
  config.depthState.depthTestEnable = true;
  config.depthState.depthWriteEnable = true;
  config.depthState.depthCompareOp = RHI::CompareOp::Less;

  // Formats
  config.colorFormat = RHI::TextureFormat::B8G8R8A8_SRGB;
  config.depthFormat = RHI::TextureFormat::D32_FLOAT;
  config.hasDepthAttachment = true;

  // needed for zapping with lightning
  config.pushConstants.size =
      sizeof(glm::vec4); // Just passing targetPos + padding
  config.pushConstants.offset = 0;
  config.pushConstants.stage = RHI::ShaderStage::Fragment;

  pipeline = device.createPipeline("Terrain_vs", "Terrain_fs", config);
}

void TerrainPass::onResize(uint32_t newWidth, uint32_t newHeight) {
  if (newWidth == 0 || newHeight == 0)
    return;

  camera.setAspectRatio(static_cast<float>(newWidth),
                        static_cast<float>(newHeight));
  createDepthTarget(newWidth, newHeight);
}

void TerrainPass::update(WindowHandling &window, float deltaTime,
                         float totalTime, uint32_t frameIndex) {
  // Process interactive WASD + QE input
  camera.processKeyboardInput(window.getGLFWwindow(), deltaTime);

  // Trigger camera matrix update
  camera.update(deltaTime, totalTime);

  // Upload updated ViewProjection matrix to GPU UBO
  void *mappedUBO = cameraUniformBuffers[frameIndex]->map();
  std::memcpy(mappedUBO, &camera.getFrameData(), sizeof(Core::CameraFrameData));
  cameraUniformBuffers[frameIndex]->unmap();
}

void TerrainPass::cull(RHI::CommandList &commandList, uint32_t frameIndex) {
  commandList.beginDebugMarker("Terrain GPU Culling", nullptr);

  commandList.transitionBuffer(chunkDataBuffer.get(),
                               RHI::ResourceState::ShaderResource);
  commandList.transitionBuffer(indirectDrawBuffers[frameIndex].get(),
                               RHI::ResourceState::UnorderedAccess);

  commandList.bindPipeline(*cullPipeline);
  commandList.bindStorageBuffer(0, chunkDataBuffer.get());
  commandList.bindUniformBuffer(1, cameraUniformBuffers[frameIndex].get());
  commandList.bindStorageBuffer(2, indirectDrawBuffers[frameIndex].get());

  commandList.pushConstants(0, sizeof(uint32_t), &totalChunks,
                            RHI::ShaderStage::Compute);

  uint32_t groupCount = (totalChunks + 255) / 256;
  commandList.dispatch(groupCount, 1, 1);

  // Transition to command execution state!
  commandList.transitionBuffer(indirectDrawBuffers[frameIndex].get(),
                               RHI::ResourceState::IndirectArgument);

  commandList.endDebugMarker();
}

void TerrainPass::render(RHI::CommandList &commandList,
                         RHI::Texture *targetColorTexture, uint32_t width,
                         uint32_t height, uint32_t frameIndex,
                         glm::vec3 targetPos) {

  const float colorTerrain[4] = {0.35f, 0.72f, 0.35f, 1.0f};
  commandList.beginDebugMarker("Terrain Geometry Pass", colorTerrain);

  glm::vec4 pushTarget = glm::vec4(targetPos.x, targetPos.y, targetPos.z, 0.0f);

  commandList.transitionBuffer(vertexBuffer.get(),
                               RHI::ResourceState::ShaderResource);

  // Viewport & Scissor not hardcoded like before since the window can change
  // size now
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(width),
                          static_cast<float>(height));
  commandList.setScissor(0, 0, width, height);

  commandList.bindPipeline(*pipeline);

  // Bindings
  commandList.bindStorageBuffer(0, vertexBuffer.get());
  commandList.bindUniformBuffer(1, cameraUniformBuffers[frameIndex].get());
  commandList.pushConstants(0, sizeof(glm::vec4), &pushTarget,
                            RHI::ShaderStage::Fragment);

  commandList.bindIndexBuffer(indexBuffer.get(), RHI::IndexType::Uint32, 0);
  commandList.drawIndexedIndirect(indirectDrawBuffers[frameIndex].get(), 0,
                                  totalChunks,
                                  sizeof(RHI::DrawIndexedIndirectCommand));

  commandList.endDebugMarker();
}

} // namespace elementalEngine::Graphics
