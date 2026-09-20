struct FireParticles {
  float4 position;
  float4 velocity;

  float life;
  float maxLife;
  float temperature; // core 1.0, 0.0 cold (should I transition to smoke?)
  float particleRadius;
};

struct FireRenderParams {
  float4x4 viewMatrix;
  float4x4 projMatrix;

  float domainWidth;
  float worldSizeX;
  float2 pad;
};

[[vk::push_constant]] FireRenderParams renderParams;

StructuredBuffer<FireParticles> particles : register(t0);

struct VSOut {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
  float temperature : TEXCOORD1;
  float lifeRatio : TEXCOORD2;
};

static float2 QUAD_VERTS[6] = {float2(-1.0f, -1.0f), float2(1.0f, -1.0f),
                               float2(-1.0f, 1.0f),  float2(-1.0f, 1.0f),
                               float2(1.0f, -1.0f),  float2(1.0f, 1.0f)};

static float2 QUAD_UVS[6] = {float2(0.0, 0.0), float2(1.0, 0.0),
                             float2(0.0, 1.0), float2(0.0, 1.0),
                             float2(1.0, 0.0), float2(1.0, 1.0)};

VSOut VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
  VSOut output;
  FireParticles p = particles[instanceID];

  float radius = (p.life > 0.0f) ? p.particleRadius : 0.0f;

  float2 localPos = QUAD_VERTS[vertexID];
  output.uv = QUAD_UVS[vertexID];

  float scaleX = renderParams.worldSizeX / renderParams.domainWidth;

  float3 scaledWorldPos = p.position.xyz * scaleX;
  float scaledRadius = radius * scaleX;

  float4 viewCenter =
      mul(renderParams.viewMatrix, float4(scaledWorldPos, 1.0f));

  float4 viewPos = float4(
      viewCenter.xyz + float3(localPos.x, localPos.y, 0.0f) * scaledRadius,
      1.0f);

  output.position = mul(renderParams.projMatrix, viewPos);

  output.temperature = p.temperature;
  output.lifeRatio = saturate(p.life / p.maxLife);

  return output;
}