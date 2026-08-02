struct Particle {
  float2 position;
  float2 velocity;
  float2 predictedPosition;
  float density;
  float nearDensity;
  float pressure;
  float nearPressure;
  float health;
  float pad;
};

struct PBFRenderParams {
  float4x4 viewProj;
  float particleRadius;
  float3 pad0;
};

// even though from this point I have decided to focus on Vulkan and not DX12
// anymore but for now I keep the DX12 related code until i clean up the whole
// repository
#ifdef __SPIRV__
[[vk::push_constant]] PBFRenderParams renderParams;
#else
ConstantBuffer<PBFRenderParams> renderParams : register(b0);
#endif

StructuredBuffer<Particle> particles : register(t0);

struct VSOut {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
  float health : TEXCOORD1;
};

static float2 QUAD_VERTS[6] = {float2(-1.0f, -1.0f), float2(1.0f, -1.0f),
                               float2(-1.0f, 1.0f),  float2(-1.0f, 1.0f),
                               float2(1.0f, -1.0f),  float2(1.0f, 1.0f)};

static float2 QUAD_UVS[6] = {float2(0.0, 0.0), float2(1.0, 0.0),
                             float2(0.0, 1.0), float2(0.0, 1.0),
                             float2(1.0, 0.0), float2(1.0, 1.0)};

VSOut VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
  VSOut output;
  Particle p = particles[instanceID];

  float2 localPos = QUAD_VERTS[vertexID];
  output.uv = QUAD_UVS[vertexID];

  float activeRadius = renderParams.particleRadius * saturate(p.health);

  float2 worldPos = p.position + (localPos * activeRadius);

  output.position = mul(renderParams.viewProj, float4(worldPos, 0.0f, 1.0f));

  output.health = p.health;

  return output;
}
