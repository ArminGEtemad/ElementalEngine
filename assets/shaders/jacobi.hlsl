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
Texture2D<float> ReadObstacle : register(t3);
RWTexture2D<float> WritePressure : register(u4);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight) return;

    uint left = id.x > 0 ? id.x - 1 : id.x;
    uint right = id.x < SimConfig.gridWidth - 1 ? id.x + 1 : id.x;
    uint top = id.y > 0 ? id.y - 1 : id.y;
    uint bottom = id.y < SimConfig.gridHeight - 1 ? id.y + 1 : id.y;

    float pC = ReadPressure[id.xy];
    float oL = ReadObstacle[uint2(left, id.y)];
    float oR = ReadObstacle[uint2(right, id.y)];
    float oT = ReadObstacle[uint2(id.x, top)];
    float oB = ReadObstacle[uint2(id.x, bottom)];

    // If neighbor is a wall, use center pressure
    float pL = oL > 0.5f ? pC : ReadPressure[uint2(left, id.y)];
    float pR = oR > 0.5f ? pC : ReadPressure[uint2(right, id.y)];
    float pT = oT > 0.5f ? pC : ReadPressure[uint2(id.x, top)];
    float pB = oB > 0.5f ? pC : ReadPressure[uint2(id.x, bottom)];
    float b = ReadDivergence[id.xy];

    // Jacobi Iteration Formula for Poisson Equation
    WritePressure[id.xy] = (pL + pR + pB + pT - b) * 0.25f;
}