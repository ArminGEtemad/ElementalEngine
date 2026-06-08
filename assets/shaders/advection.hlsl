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

    if (x >= SimConfig.gridWidth || y >= SimConfig.gridHeight) return;

    uint index = y * SimConfig.gridWidth + x;

    // -----------test vis-----------------
    // constant wind
    float2 currentVelocity = float2(50.0f, -20.0f);

    // a gas emitter
    if (x > 20 && x < 30 && y > SimConfig.gridHeight / 2 - 10 && y < SimConfig.gridHeight / 2 + 10) {
        WriteDensity[index] = 1.0f; // inject pure density
        return; // skip advection for the emitter itself
    }
    // ------------------------------------

    float srcX = (float)x - (currentVelocity.x * SimConfig.dt);
    float srcY = (float)y - (currentVelocity.y * SimConfig.dt);

    srcX = clamp(srcX, 0.0f, (float)SimConfig.gridWidth - 1.0f);
    srcY = clamp(srcY, 0.0f, (float)SimConfig.gridHeight - 1.0f);

    uint srcIndex = (uint)round(srcY) * SimConfig.gridWidth + (uint)round(srcX);
    WriteDensity[index] = ReadDensity[srcIndex];
}