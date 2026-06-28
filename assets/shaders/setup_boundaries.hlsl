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

RWTexture2D<float> WriteObstacle : register(u4); 

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight) return;

    float isObstacle = 0.0f; // empty

    // walls and floor, and ceiling
    if (id.x <= 2 || id.x >= SimConfig.gridWidth - 3) isObstacle = 1.0f;
    if (id.y <= 2 || id.y >= SimConfig.gridHeight - 3) isObstacle = 1.0f;

// Splash Platform for the acid projectile later
    int centerX = SimConfig.gridWidth / 2;
    int centerY = SimConfig.gridHeight / 2;
    
    int halfWidth = 30;
    int halfThickness = 2;
    float slopeM = 0.2f; 

    int relX = (int)id.x - centerX;
    int relY = (int)id.y - centerY;

    if (abs(relX) < halfWidth) {
        // y = mx
        float expectedY = slopeM * relX;

        if (abs(relY - expectedY) < halfThickness) {
            isObstacle = 1.0f;
        }
    }

    WriteObstacle[id.xy] = isObstacle;
}