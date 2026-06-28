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

Texture2D<float2> ReadVelocity : register(t1);
Texture2D<float> ReadObstacle : register(t3);
RWTexture2D<float> WriteDivergence : register(u4);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    
    if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight) return;

    // handle boundaries
    uint left = id.x > 0 ? id.x - 1 : id.x;
    uint right = id.x < SimConfig.gridWidth - 1 ? id.x + 1 : id.x;
    uint top = id.y > 0 ? id.y - 1: id.y;
    uint bottom = id.y < SimConfig.gridHeight - 1 ? id.y + 1 : id.y;

    float oL = ReadObstacle[uint2(left, id.y)];
    float oR = ReadObstacle[uint2(right, id.y)];
    float oT = ReadObstacle[uint2(id.x, top)];
    float oB = ReadObstacle[uint2(id.x, bottom)];

    // velocity is zero at the walls
    float vL = ReadVelocity[uint2(left, id.y)].x * (1.0f - oL);
    float vR = ReadVelocity[uint2(right, id.y)].x * (1.0f - oR);
    float vT = ReadVelocity[uint2(id.x, top)].y * (1.0f - oT);
    float vB = ReadVelocity[uint2(id.x, bottom)].y * (1.0f - oB);

    float divergence = 0.5f * ((vR - vL) + (vB - vT));
    WriteDivergence[id.xy] = divergence;
}