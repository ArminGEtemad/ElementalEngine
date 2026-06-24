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

struct VSOut {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 FSMain(VSOut input) : SV_TARGET {
    uint gridX = (uint)(input.uv.x * SimConfig.gridWidth);
    uint gridY = (uint)(input.uv.y * SimConfig.gridHeight);
    
    // our of bounds
    gridX = clamp(gridX, 0, SimConfig.gridWidth - 1);
    gridY = clamp(gridY, 0, SimConfig.gridHeight - 1);
    
    // 1D buffer index
    uint index = gridY * SimConfig.gridWidth + gridX;
    
    float density = ReadDensity[index];
    
    // poison gas 
    float3 gasColor = float3(0.1f, 0.9f, 0.2f);
    
    return float4(gasColor * saturate(density), 1.0f);
}