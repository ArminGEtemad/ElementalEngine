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
  glm::vec3 startPos{-15.0f, 10.0f, -15.0f};
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
  depthTexture =
      device.createTexture(width, height, RHI::TextureFormat::D32_FLOAT,
                           RHI::TextureUsage::DepthStencilAttachment);
}

void TerrainPass::createBuffers() {
  // Generate 64x64 terrain grid covering 20x20 world units
  TerrainMesh terrain = TerrainMesh::generateGrid(64, 64, 20.0f, 20.0f);
  indexCount = static_cast<uint32_t>(terrain.Indices.size());

  size_t vertexBufferSize = terrain.Vertices.size() * sizeof(RHI::Vertex3D);
  vertexBuffer =
      device.createBuffer(vertexBufferSize, RHI::BufferUsage::Storage,
                          RHI::MemoryProperty::CPUAccess);

  void *mappedVerts = vertexBuffer->map();
  std::memcpy(mappedVerts, terrain.Vertices.data(), vertexBufferSize);
  vertexBuffer->unmap();

  size_t indexBufferSize = terrain.Indices.size() * sizeof(uint32_t);
  indexBuffer = device.createBuffer(indexBufferSize, RHI::BufferUsage::Index,
                                    RHI::MemoryProperty::CPUAccess);

  void *mappedIndices = indexBuffer->map();
  std::memcpy(mappedIndices, terrain.Indices.data(), indexBufferSize);
  indexBuffer->unmap();

  cameraUniformBuffer = device.createBuffer(sizeof(Core::CameraFrameData),
                                            RHI::BufferUsage::Uniform,
                                            RHI::MemoryProperty::CPUAccess);
}

void TerrainPass::createPipeline() {
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
                         float totalTime) {
  // Process interactive WASD + QE input
  camera.processKeyboardInput(window.getGLFWwindow(), deltaTime);

  // Trigger camera matrix update
  camera.update(deltaTime, totalTime);

  // Upload updated ViewProjection matrix to GPU UBO
  void *mappedUBO = cameraUniformBuffer->map();
  std::memcpy(mappedUBO, &camera.getFrameData(), sizeof(Core::CameraFrameData));
  cameraUniformBuffer->unmap();
}

void TerrainPass::render(RHI::CommandList &commandList,
                         RHI::Texture *targetColorTexture, uint32_t width,
                         uint32_t height) {

  // Viewport & Scissor not hardcoded like before since the window can change
  // size now
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(width),
                          static_cast<float>(height));
  commandList.setScissor(0, 0, width, height);

  commandList.bindPipeline(*pipeline);

  // Bindings
  commandList.bindStorageBuffer(0, vertexBuffer.get());
  commandList.bindUniformBuffer(1, cameraUniformBuffer.get());

  // bind index buffer
  commandList.bindIndexBuffer(indexBuffer.get(), RHI::IndexType::Uint32, 0);
  commandList.drawIndexed(indexCount, 1, 0, 0, 0);
}

} // namespace elementalEngine::Graphics