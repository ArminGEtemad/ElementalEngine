struct Particle {
    float2 position;
    float2 velocity;
    float2 predictedPosition;
    uint state; // 0: flying 1: settled
    float lambda;
};

struct PBFRenderParams {
float4x4 viewProj;
  float particleRadius;
  float3 pad0;
};

#ifdef __SPIRV__
[[vk::push_constant]] PBFRenderParams renderParams;
#else
ConstantBuffer<PBFRenderParams> renderParams : register(b0);
#endif

StructuredBuffer<Particle> particles : register(t0);

struct VSOut {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    nointerpolation uint state : BLENDINDICES0;
};

static float2 QUAD_VERTS[6] = {
    float2(-1.0f, -1.0f), float2(1.0f, -1.0f), float2(-1.0f, 1.0f),
    float2(-1.0f, 1.0f), float2(1.0f, -1.0f), float2(1.0f, 1.0f)
};

// give it to fragment shader to make circle next
static float2 QUAD_UVS[6] = {
    float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0),
    float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)
};

VSOut VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
    VSOut output;
    Particle p = particles[instanceID];
    
    float2 localPos = QUAD_VERTS[vertexID];
    output.uv = QUAD_UVS[vertexID];
    output.state = p.state;

    float2 worldPos = p.position + (localPos * renderParams.particleRadius);

    output.position = mul(renderParams.viewProj, float4(worldPos, 0.0f, 1.0f));

    return output;
}
