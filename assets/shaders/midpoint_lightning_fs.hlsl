struct LightningRenderParams {
    float4x4 viewProj;
    float opacity;
    float thickness;
    float2 pad;
};

#ifdef __SPIRV__
[[vk::push_constant]] LightningRenderParams renderParams;
#else
ConstantBuffer<LightningRenderParams> renderParams : register(b0);
#endif

struct VSOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 FSMain(VSOut input) : SV_Target {
    // Calculate the distance of the pixel from the center of the ribbon
    float distFromCenter = abs(input.uv.y);
    
    // Exponential falloff for a soft electric glow
    float glow = exp(-distFromCenter * distFromCenter * 5.0f); 

    // Electric neon
    float3 coreColor = float3(1.0f, 1.0f, 1.0f); // white core
    float3 outerGlowColor = float3(0.584f, 0.0f, 1.0f); // cyan electric should I do violet neon?

    // Interpolate from white core to blue glow
    float3 finalColor = lerp(outerGlowColor, coreColor, glow);
    
    // Multiply color by the glow intensity and the fade opacity push constant
    return float4(finalColor * glow, glow * renderParams.opacity);
}