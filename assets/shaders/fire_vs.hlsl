struct FireParticles {
    float2 position;
    float2 velocity;
    float life;
    float maxLife;
    float temperature; // core 1.0, 0.0 cold (should I transition to smoke?)
    float particleRadius;
};

struct FireRenderParams {
    float4x4 viewProj;
};

#ifdef __SPIRV__
[[vk::push_constant]] FireRenderParams renderParams;
#else
ConstantBuffer<FireRenderParams> renderParams : register(b0);
#endif

StructuredBuffer<FireParticles> particles : register(t0);

struct VSOut {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float temperature : TEXCOORD1;
    float lifeRatio: EXTRA0;
};

static float2 QUAD_VERTS[6] = {
    float2(-1.0f, -1.0f), float2(1.0f, -1.0f), float2(-1.0f, 1.0f),
    float2(-1.0f, 1.0f), float2(1.0f, -1.0f), float2(1.0f, 1.0f)
};

static float2 QUAD_UVS[6] = {
    float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0),
    float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)
};

VSOut VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
    VSOut output;
    FireParticles p = particles[instanceID];

    float radius = (p.life > 0.0f) ? p.particleRadius : 0.0f;

    float2 localPos = QUAD_VERTS[vertexID];
    output.uv = QUAD_UVS[vertexID];

    float2 worldPos = p.position + (localPos * radius);
    output.position = mul(renderParams.viewProj, float4(worldPos, 0.0f, 1.0f));

    output.temperature = p.temperature;
    output.lifeRatio = saturate(p.life / p.maxLife);

    return output;
}