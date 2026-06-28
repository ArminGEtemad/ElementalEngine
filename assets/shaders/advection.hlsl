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

Texture2D<float> ReadDensity : register(t1);   
Texture2D<float2> ReadVelocity : register(t2);
Texture2D<float> ReadObstacle : register(t3);
RWTexture2D<float> WriteDensity : register(u4);
RWTexture2D<float2> WriteVelocity : register(u5);

SamplerState LinearSampler : register(s6);

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint x = dispatchThreadID.x;
    uint y = dispatchThreadID.y;

    if (x >= SimConfig.gridWidth || y >= SimConfig.gridHeight) return;
    uint2 index2D = uint2(x, y);
    float2 currentVelocity = ReadVelocity[index2D];

    // trace back using current velocity
    float srcX = (float)x - (currentVelocity.x * SimConfig.dt);
    float srcY = (float)y - (currentVelocity.y * SimConfig.dt);

    // uv coordinate
    float2 uv = float2((srcX + 0.5f) / (float)SimConfig.gridWidth, 
                       (srcY + 0.5f) / (float)SimConfig.gridHeight);

    float newDensity = ReadDensity.SampleLevel(LinearSampler, uv, 0) * 0.9995f;
    float2 newVelocity = ReadVelocity.SampleLevel(LinearSampler, uv, 0) * 0.999f;
    
    // External Force
    newVelocity.y += (newDensity * SimConfig.forceY * SimConfig.dt);

    // Gas Emitter
    int cx = SimConfig.gridWidth / 2;
    int cy = 3; 
    
    if (abs((int)x - cx) < 2 && abs((int)y - cy) < 2) {
        newDensity = 1.0f; // Continuously inject density
    }

    if (x <= 1 || x >= SimConfig.gridWidth - 2) newVelocity.x = 0.0f;
    if (y <= 1 || y >= SimConfig.gridHeight - 2) newVelocity.y = 0.0f;



    WriteDensity[index2D] = newDensity;
    WriteVelocity[index2D] = newVelocity;
}