struct LightningRenderParams {
  float4x4 viewProj;
  float4 cameraPos;
  float opacity;
  float thickness;
  float2 pad;
};

[[vk::push_constant]] LightningRenderParams renderParams;

struct VSOut {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
  float scale : TEXCOORD1;
};

float4 FSMain(VSOut input) : SV_Target {
  // Calculate the distance of the pixel from the center of the ribbon
  float distFromCenter = abs(input.uv.y);

  // Exponential falloff
  float glow = exp(-distFromCenter * distFromCenter * 5.0f);

  float3 coreColor = float3(1.0f, 1.0f, 1.0f);        // white core
  float3 outerGlowColor = float3(0.584f, 0.0f, 1.0f); // neon violet

  // Interpolate from white core to violet glow
  float3 finalColor = lerp(outerGlowColor, coreColor, glow);

  float finalOpacity = glow * renderParams.opacity * input.scale;

  // Multiply color by the glow intensity and the fade opacity push constant
  return float4(finalColor * glow, finalOpacity);
}