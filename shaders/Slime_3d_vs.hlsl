struct Particle {
  float4 position;
  float4 velocity;
  float4 predictedPosition;

  float density;
  float nearDensity;
  float pressure;
  float nearPressure;
};

// Storage Buffer containing physics particles

StructuredBuffer<Particle> Particles : register(t0);

struct PushConstants {
  float4x4 ViewProj;
  float DomainWidth;
  float DomainHeight;
  float WorldSizeX;
  float WorldSizeZ;
  float ParticleRadius;
};

[[vk::push_constant]] PushConstants pushConstants;

struct VSOutput {
  float4 posClip : SV_POSITION;
  float2 uv : TEXCOORD0;
};

// Entry point named VSMain to match CMake -E VSMain
VSOutput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
  VSOutput output;
  float3 pos3D = Particles[instanceID].position.xyz;

  // scale from 2D to 3D domain
  float scaleX = pushConstants.WorldSizeX / pushConstants.DomainWidth;

  float worldX = pos3D.x * scaleX;
  float worldY = pos3D.y * scaleX;
  float worldZ = pos3D.z * scaleX;

  float3 centerWorld = float3(worldX, worldY, worldZ);

  static const float2 QUAD_OFFSETS[6] = {float2(-1.0, -1.0), float2(1.0, -1.0),
                                         float2(-1.0, 1.0),  float2(-1.0, 1.0),
                                         float2(1.0, -1.0),  float2(1.0, 1.0)};

  float2 offset = QUAD_OFFSETS[vertexID];
  output.uv = offset;

  // normalize the camera vectors to prevent the ViewProj matrix from
  // squashing the blobs
  float3 camRight =
      normalize(float3(pushConstants.ViewProj._11, pushConstants.ViewProj._21,
                       pushConstants.ViewProj._31));
  float3 camUp =
      normalize(float3(pushConstants.ViewProj._12, pushConstants.ViewProj._22,
                       pushConstants.ViewProj._32));

  float3 worldPos = centerWorld + (camRight * offset.x + camUp * offset.y) *
                                      pushConstants.ParticleRadius;

  output.posClip = mul(pushConstants.ViewProj, float4(worldPos, 1.0));

  return output;
}