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

Texture2D<float> ReadPressure : register(t1);
Texture2D<float> ReadDivergence : register(t2);
RWTexture2D<float> WritePressure : register(u3);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight) return;

    uint left = id.x > 0 ? id.x - 1 : id.x;
    uint right = id.x < SimConfig.gridWidth - 1 ? id.x + 1 : id.x;
    uint top = id.y > 0 ? id.y - 1 : id.y;
    uint bottom = id.y < SimConfig.gridHeight - 1 ? id.y + 1 : id.y;

    float pL = ReadPressure[uint2(left, id.y)];
    float pR = ReadPressure[uint2(right, id.y)];
    float pT = ReadPressure[uint2(id.x, top)];
    float pB = ReadPressure[uint2(id.x, bottom)];
    float b = ReadDivergence[id.xy];

    // Jacobi Iteration Formula for Poisson Equation
    WritePressure[id.xy] = (pL + pR + pB + pT - b) * 0.25f;
}