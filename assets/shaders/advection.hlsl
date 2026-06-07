struct SimConfigStruct {
    uint gridWidth;
    uint gridHeight;
    float dt;
    float pad_0;
};

#ifdef __SPIRV__
[[vk::push_constant]] SimConfigStruct SimConfig;
#else
ConstantBuffer<SimConfigStruct> SimConfig : register(b0);
#endif

// ping pong buffer
StructuredBuffer<float> ReadDensity : register(t1); // old state
StructuredBuffer<float> ReadVelocity : register(t2); // Velocity field
RWStructuredBuffer<float> WriteDensity : register(u3); // new state

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint x = dispatchThreadID.x;
    uint y = dispatchThreadID.y;

    // Boundary
    if (x >= SimConfig.gridWidth || y >= SimConfig.gridHeight) return;

    // index calculation
    uint index = y * SimConfig.gridWidth + x;

    // ------------------------
    float2 currentVelocity = ReadVelocity[index];

    float newX = (float)x - (currentVelocity.x * SimConfig.dt);
    float newY = (float)y - (currentVelocity.y * SimConfig.dt);

    newX = clamp(newX, 0.0f, (float)SimConfig.gridWidth - 1.0f);
    newY = clamp(newY, 0.0f, (float)SimConfig.gridHeight - 1.0f);

    uint newIndex = (uint)round(newY) * SimConfig.gridWidth + (uint)round(newX);

    WriteDensity[index] = ReadDensity[newIndex];
}