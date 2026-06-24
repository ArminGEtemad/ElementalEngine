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

StructuredBuffer<float> ReadPressure : register(t1);
StructuredBuffer<float2> ReadVelocity : register(t2);
RWStructuredBuffer<float2> WriteVelocity : register(u3);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight) return;

    uint index = id.y * SimConfig.gridWidth + id.x;

    // Handle boundaries safely
    uint left = id.x > 0 ? id.y * SimConfig.gridWidth + (id.x - 1) : index;
    uint right = id.x < SimConfig.gridWidth - 1 ? id.y * SimConfig.gridWidth + (id.x + 1) : index;
    uint top = id.y > 0 ? (id.y - 1) * SimConfig.gridWidth + id.x : index;
    uint bottom = id.y < SimConfig.gridHeight - 1 ? (id.y + 1) * SimConfig.gridWidth + id.x : index;

    float pL = ReadPressure[left];
    float pR = ReadPressure[right];
    float pT = ReadPressure[top];
    float pB = ReadPressure[bottom];

    float2 currentVel = ReadVelocity[index];
    
    // Subtract the pressure gradient
    float2 newVel = currentVel - float2(pR - pL, pB - pT) * 0.5f;
    WriteVelocity[index] = newVel;
}