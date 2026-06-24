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

StructuredBuffer<float> ReadDensity : register(t1);     
StructuredBuffer<float2> ReadVelocity : register(t2);    
RWStructuredBuffer<float> WriteDensity : register(u3);
RWStructuredBuffer<float2> WriteVelocity : register(u4); 

uint getIndex(int x, int y) {
    x = clamp(x, 0, (int)SimConfig.gridWidth - 1);
    y = clamp(y, 0, (int)SimConfig.gridHeight - 1);
    return y * SimConfig.gridWidth + x;
}

// TODO refactor using texture?
float bilerpDensity(float x, float y) {
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float tx = x - (float)x0;
    float ty = y - (float)y0;

    float c00 = ReadDensity[getIndex(x0, y0)];
    float c10 = ReadDensity[getIndex(x1, y0)];
    float c01 = ReadDensity[getIndex(x0, y1)];
    float c11 = ReadDensity[getIndex(x1, y1)];

    float top = lerp(c00, c10, tx);
    float bottom = lerp(c01, c11, tx);
    return lerp(top, bottom, ty);
}

// TODO refactor using texture?
float2 bilerpVelocity(float x, float y) {
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float tx = x - (float)x0;
    float ty = y - (float)y0;

    float2 c00 = ReadVelocity[getIndex(x0, y0)];
    float2 c10 = ReadVelocity[getIndex(x1, y0)];
    float2 c01 = ReadVelocity[getIndex(x0, y1)];
    float2 c11 = ReadVelocity[getIndex(x1, y1)];

    float2 top = lerp(c00, c10, tx);
    float2 bottom = lerp(c01, c11, tx);
    return lerp(top, bottom, ty);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint x = dispatchThreadID.x;
    uint y = dispatchThreadID.y;

    if (x >= SimConfig.gridWidth || y >= SimConfig.gridHeight) return;
    uint index = y * SimConfig.gridWidth + x;

    // trace back using current velocity
    float2 currentVelocity = ReadVelocity[index];
    float srcX = clamp((float)x - (currentVelocity.x * SimConfig.dt), 0.0f, (float)SimConfig.gridWidth - 1.0f);
    float srcY = clamp((float)y - (currentVelocity.y * SimConfig.dt), 0.0f, (float)SimConfig.gridHeight - 1.0f);

    // bilinear Advection (The Critical Fix)
    float newDensity = bilerpDensity(srcX, srcY) * 0.9999f;
    float2 newVelocity = bilerpVelocity(srcX, srcY) * 0.999f;

    // External Force
    newVelocity.y += (newDensity * SimConfig.forceY * SimConfig.dt);

    // Gas Emitter
    int cx = SimConfig.gridWidth / 2;
    int cy = 3; 
    
    if (abs((int)x - cx) < 2 && abs((int)y - cy) < 2) {
        newDensity = 1.0f; // Continuously inject density
    }

    WriteDensity[index] = newDensity;
    WriteVelocity[index] = newVelocity;
}