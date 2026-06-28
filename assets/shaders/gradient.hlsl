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
Texture2D<float2> ReadVelocity : register(t2);
RWTexture2D<float2> WriteVelocity : register(u3);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight) return;

    // Handle boundaries safely
    uint left = id.x > 0 ? id.x - 1 : id.x;
    uint right = id.x < SimConfig.gridWidth - 1 ? id.x + 1: id.x;
    uint top = id.y > 0 ? id.y - 1 : id.y;
    uint bottom = id.y < SimConfig.gridHeight - 1 ? id.y + 1: id.y;

    float pL = ReadPressure[uint2(left, id.y)];
    float pR = ReadPressure[uint2(right, id.y)];
    float pT = ReadPressure[uint2(id.x, top)];
    float pB = ReadPressure[uint2(id.x, bottom)];

    float2 vel = ReadVelocity[id.xy];
    vel.x -= 0.5f * (pR - pL);
    vel.y -= 0.5f * (pB - pT);

    WriteVelocity[id.xy] = vel;
}