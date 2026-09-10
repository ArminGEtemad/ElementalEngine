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
  // splitting view projection into two steps view matrix and projection matrix
  float4x4 ViewMatrix;
  float4x4 projMatrix;

  float DomainWidth;
  float DomainHeight;
  float WorldSizeX;
  float WorldSizeZ;

  float ParticleRadius;
  float3 pad;
};

StructuredBuffer<Particle> Particles : register(t0);
[[vk::push_constant]] PushConstants pushConstants;

struct VSOutput {
  float4 posClip : SV_POSITION;
  float3 posView : TEXCOORD0;
  float2 uv : TEXCOORD1;
  float health : TEXCOORD2;
};

VSOutput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
  VSOutput output;
  float health = Particles[instanceID].health;

  // don't draw dead particles anymore
  if (health <= 0.0f) {
    output.posClip = float4(0, 0, 0, 0);
    return output;
  }

  float3 pos3D = Particles[instanceID].position.xyz;
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

  float4 viewCenter = mul(pushConstants.ViewMatrix, float4(centerWorld, 1.0));
  float currentRadius = pushConstants.ParticleRadius * health;

  float3 viewPos =
      viewCenter.xyz + float3(offset.x, offset.y, 0.0) * currentRadius;

  output.posView = viewPos;
  output.posClip = mul(pushConstants.projMatrix, float4(viewPos, 1.0));

  output.health = health;

  return output;
}
