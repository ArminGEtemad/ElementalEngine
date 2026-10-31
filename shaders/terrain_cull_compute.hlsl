static const uint THREAD_GROUP_SIZE = 256;

struct TerrainChunkData {
  float4 boundsMin;
  float4 boundsMax;
  uint firstIndex;
  uint indexCount;
  float2 pad;
};

struct DrawIndexedIndirectCommand {
  uint indexCount;
  uint instanceCount;
  uint firstIndex;
  int vertexOffset;
  uint firstInstance;
};

struct CameraFrameData {
  float4x4 viewMatrix;
  float4x4 projectionMatrix;
  float4x4 viewProjection;
  float4x4 invViewMatrix;
  float4x4 invProjectionMatrix;
  float4x4 invViewProjection;
  float4 cameraPosition;
  float time;
  float deltaTime;
  float2 padding;
};

[[vk::push_constant]] struct {
  uint totalChunks;
} pushData;

StructuredBuffer<TerrainChunkData> chunks : register(t0);
ConstantBuffer<CameraFrameData> camera : register(b1);
RWStructuredBuffer<DrawIndexedIndirectCommand> indirectDraws : register(u2);

bool isAABBVisible(float4 minExtents, float4 maxExtents, float4x4 VP) {
  // 6 frustum
  float4 planes[6];
  planes[0] = VP[3] + VP[0]; // Left
  planes[1] = VP[3] - VP[0]; // Right
  planes[2] = VP[3] + VP[1]; // Bottom
  planes[3] = VP[3] - VP[1]; // Top
  planes[4] = VP[2];         // Near
  planes[5] = VP[3] - VP[2]; // Far

  for (int i = 0; i < 6; ++i) {
    float4 plane = planes[i];

    // Find the corner furthest in the direction of the plane's
    // normal
    float3 p = minExtents.xyz;
    if (plane.x > 0.0f)
      p.x = maxExtents.x;
    if (plane.y > 0.0f)
      p.y = maxExtents.y;
    if (plane.z > 0.0f)
      p.z = maxExtents.z;

    // If it is outside the plane, the ENTIRE box is outside the
    // frustum!
    if (dot(plane.xyz, p) + plane.w < 0.0f) {
      return false;
    }
  }
  return true;
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)] void
CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint id = DTid.x;
  if (id >= pushData.totalChunks)
    return;

  TerrainChunkData chunk = chunks[id];

  DrawIndexedIndirectCommand cmd;
  cmd.indexCount = chunk.indexCount;
  cmd.instanceCount = 0;
  cmd.firstIndex = chunk.firstIndex;
  cmd.vertexOffset = 0;
  cmd.firstInstance = 0;

  if (isAABBVisible(chunk.boundsMin, chunk.boundsMax, camera.viewProjection)) {
    cmd.instanceCount = 1;
  }

  indirectDraws[id] = cmd;
}
