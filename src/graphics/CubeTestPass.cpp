#include "CubeTestPass.hpp"
#include "CubeMesh.hpp"
#include <cstring>

namespace elementalEngine::Graphics {

CubeTestPass::CubeTestPass(RHI::Device &device, RHI::Swapchain &swapchain)
    : device(device), camera(60.0f,
                             static_cast<float>(swapchain.getWidth()) /
                                 static_cast<float>(swapchain.getHeight()),
                             0.1f, 1000.0f) {

  // Position camera 3 units back along +Z looking down -Z at origin
  // the other way around the cube was behind the camera
  camera.setPosition({0.0f, 0.0f, -3.0f});

  createDepthTarget(swapchain.getWidth(), swapchain.getHeight());
  createBuffers();
  createPipeline();
}

void CubeTestPass::createDepthTarget(uint32_t width, uint32_t height) {
  depthTexture =
      device.createTexture(width, height, RHI::TextureFormat::D32_FLOAT,
                           RHI::TextureUsage::DepthStencilAttachment);
}

void CubeTestPass::createBuffers() {
  // vertex pulling sends vertecies to gpu
  size_t vertexBufferSize = CubeMesh::Vertices.size() * sizeof(RHI::Vertex3D);
  vertexBuffer =
      device.createBuffer(vertexBufferSize, RHI::BufferUsage::Storage,
                          RHI::MemoryProperty::CPUAccess);

  void *mappedVerts = vertexBuffer->map();
  std::memcpy(mappedVerts, CubeMesh::Vertices.data(), vertexBufferSize);
  vertexBuffer->unmap();

  // send Index Buffer
  size_t indexBufferSize = sizeof(uint32_t) * CubeMesh::Indices.size();
  indexBuffer = device.createBuffer(indexBufferSize, RHI::BufferUsage::Index,
                                    RHI::MemoryProperty::CPUAccess);

  void *mappedIndices = indexBuffer->map();
  std::memcpy(mappedIndices, CubeMesh::Indices.data(), indexBufferSize);
  indexBuffer->unmap();

  // allocate Camera FrameData Uniform Buffer
  cameraUniformBuffer = device.createBuffer(sizeof(Core::CameraFrameData),
                                            RHI::BufferUsage::Uniform,
                                            RHI::MemoryProperty::CPUAccess);
}

void CubeTestPass::createPipeline() {
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
  config.cullMode = RHI::CullMode::Back;
  config.depthState.depthTestEnable = true;
  config.depthState.depthWriteEnable = true;
  config.depthState.depthCompareOp = RHI::CompareOp::Less;

  // Formats
  config.colorFormat = RHI::TextureFormat::B8G8R8A8_SRGB;
  config.depthFormat = RHI::TextureFormat::D32_FLOAT;
  config.hasDepthAttachment = true;

  pipeline = device.createPipeline("Cube_vs", "Cube_fs", config);
}

void CubeTestPass::onResize(uint32_t newWidth, uint32_t newHeight) {
  if (newWidth == 0 || newHeight == 0)
    return;

  camera.setAspectRatio(static_cast<float>(newWidth),
                        static_cast<float>(newHeight));
  createDepthTarget(newWidth, newHeight);
}

void CubeTestPass::update(float deltaTime, float totalTime) {
  // camera rotation to make sure everything is wired correctly
  float radius = 3.5f;
  float camX = std::sin(totalTime * 0.8f) * radius;
  float camZ = std::cos(totalTime * 0.8f) * radius;

  camera.setPosition({camX, 1.2f, camZ});

  // Look toward origin (0, 0, 0)
  glm::vec3 direction = glm::normalize(glm::vec3(0.0f) - camera.getPosition());
  glm::quat lookAtRot = glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f));
  camera.setOrientation(lookAtRot);

  camera.update(deltaTime, totalTime);

  // Update GPU Uniform Buffer
  void *mappedUBO = cameraUniformBuffer->map();
  std::memcpy(mappedUBO, &camera.getFrameData(), sizeof(Core::CameraFrameData));
  cameraUniformBuffer->unmap();
}

void CubeTestPass::render(RHI::CommandList &commandList,
                          RHI::Texture *targetColorTexture, uint32_t width,
                          uint32_t height) {
  // Construct dynamic rendering info
  RHI::RenderingInfo renderingInfo{};
  renderingInfo.renderWidth = width;
  renderingInfo.renderHeight = height;

  // beckground color
  RHI::RenderPassAttachment colorAttachment{};
  colorAttachment.texture = targetColorTexture;
  colorAttachment.clear = true;
  colorAttachment.clearColor[0] = 0.3f;
  colorAttachment.clearColor[1] = 0.7f;
  colorAttachment.clearColor[2] = 0.6f;
  colorAttachment.clearColor[3] = 1.0f;
  renderingInfo.colorAttachments.push_back(colorAttachment);

  // Depth Attachment
  RHI::DepthAttachment depthAttachment{};
  depthAttachment.texture = depthTexture.get();
  depthAttachment.clear = true;
  depthAttachment.clearDepth = 1.0f;
  depthAttachment.clearStencil = 0;
  renderingInfo.depthAttachment = depthAttachment;

  // begin rendering
  commandList.beginRendering(renderingInfo);

  // Viewport & Scissor not hardcoded like before since the window can change
  // size now
  commandList.setViewport(0.0f, 0.0f, static_cast<float>(width),
                          static_cast<float>(height));
  commandList.setScissor(0, 0, width, height);

  commandList.bindPipeline(*pipeline);

  // Bindings
  commandList.bindStorageBuffer(0, vertexBuffer.get());        // register(t0)
  commandList.bindUniformBuffer(1, cameraUniformBuffer.get()); // register(b1)

  // bind index buffer
  commandList.bindIndexBuffer(indexBuffer.get(), RHI::IndexType::Uint32, 0);
  commandList.drawIndexed(36, 1, 0, 0, 0);

  // end rendering
  commandList.endRendering();
}

} // namespace elementalEngine::Graphics