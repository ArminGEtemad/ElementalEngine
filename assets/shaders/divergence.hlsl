struct SimConfigStruct {
    uint gridWidth;
    uint gridHeight;
    float dt;
    float forceY;
};

#ifdef __SPIRV__
[[vk::push_constant]] SimConfigStruct SimConfig;
#else
ConstantBuffer<SimConfigStruct> SimConfig : register(b0);
#endif

StructuredBuffer<float2> ReadVelocity : register(t1);
RWStructuredBuffer<float> WriteDivergence : register(u3);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight) return;

    uint index = id.y * SimConfig.gridWidth + id.x;

    // handle boundaries
    uint left = id.x > 0 ? id.y * SimConfig.gridWidth + (id.x - 1) : index;
    uint right = id.x < SimConfig.gridWidth - 1 ? id.y * SimConfig.gridWidth + (id.x + 1) : index;
    uint top = id.y > 0 ? (id.y - 1) * SimConfig.gridWidth + id.x : index;
    uint bottom = id.y < SimConfig.gridHeight - 1 ? (id.y + 1) * SimConfig.gridWidth + id.x : index;

    float vL = ReadVelocity[left].x;
    float vR = ReadVelocity[right].x;
    float vT = ReadVelocity[top].y;
    float vB = ReadVelocity[bottom].y;

    float divergence = 0.5f * ((vR - vL) + (vB - vT));
    WriteDivergence[index] = divergence;
}