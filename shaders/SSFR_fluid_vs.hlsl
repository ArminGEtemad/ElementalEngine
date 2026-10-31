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

struct Particle {
  float4 position;
  float4 velocity;
  float4 predictedPosition;

  float density;
  float nearDensity;
  float pressure;
  float nearPressure;

  float health;
  float3 pad;
};

struct PushConstants {
  float DomainWidth;
  float DomainHeight;
  float WorldSizeX;
  float WorldSizeZ;

  float ParticleRadius;
  float3 pad;
};

StructuredBuffer<Particle> Particles : register(t0);
StructuredBuffer<uint> AliveIndices : register(t1);
cbuffer FrameData : register(b2) { CameraFrameData camera; };
[[vk::push_constant]] PushConstants pushConstants;

struct VSOutput {
  float4 posClip : SV_POSITION;
  float3 posView : TEXCOORD0;
  float2 uv : TEXCOORD1;
  float health : TEXCOORD2;
};

VSOutput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
  VSOutput output;
  uint realID = AliveIndices[instanceID];
  float health = Particles[realID].health;

  float3 pos3D = Particles[realID].position.xyz;
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

  float4 viewCenter = mul(camera.viewMatrix, float4(centerWorld, 1.0));
  float currentRadius = pushConstants.ParticleRadius * health;

  float3 viewPos =
      viewCenter.xyz + float3(offset.x, offset.y, 0.0) * currentRadius;

  output.posView = viewPos;
  output.posClip = mul(camera.projectionMatrix, float4(viewPos, 1.0));

  output.health = health;

  return output;
}
