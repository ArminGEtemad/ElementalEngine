struct LightningRenderParams {
    float4x4 viewProj;
    float opacity;
    float thickness;
    float2 pad;
};

struct Segments {
    float2 p0;
    float2 p1;
    float scale;
    float pad;
};

#ifdef __SPIRV__
[[vk::push_constant]] LightningRenderParams renderParams;
#else
ConstantBuffer<LightningRenderParams> renderParams : register(b0);
#endif

StructuredBuffer<Segments> segments : register(t0);

struct VSOut {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float scale : TEXCOORD1;
};

static float2 QUAD_OFFSETS[6] = {
    float2(0.0f, -1.0f),
    float2(1.0f, -1.0f),
    float2(0.0f,  1.0f),
    
    float2(0.0f,  1.0f),
    float2(1.0f, -1.0f),
    float2(1.0f,  1.0f)
};


VSOut VSMain(uint vertexID : SV_VertexID) {
    VSOut output;

    // dynamic thickness
    float thickness = renderParams.thickness;

    uint segmentID = vertexID / 6;
    uint localVertexID = vertexID % 6;

    Segments s = segments[segmentID];
    float2 p0 = s.p0;
    float2 p1 = s.p1;
    output.scale = s.scale;

    // Calculate segment direction and its perpendicular normal
    float2 dir = p1 - p0;
    float len = length(dir);
    float2 perp = float2(0.0f, 0.0f);
    if (len > 1e-5f) {
        perp = float2(-dir.y, dir.x) / len;
    }

    // Determine the base position and offset for this quad vertex
    float2 offsetMode = QUAD_OFFSETS[localVertexID];
    float2 basePos = (offsetMode.x == 0.0f) ? p0 : p1;
    float2 worldPos = basePos + perp * (offsetMode.y * thickness);

    output.position = mul(renderParams.viewProj, float4(worldPos, 0.0f, 1.0f));
    output.uv = offsetMode;
    
    return output;
}