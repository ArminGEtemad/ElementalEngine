struct VSOutput {
  float4 position : SV_Position;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD0;
};

float4 FSMain(VSOutput input) : SV_Target {
  float3 lightDir = normalize(float3(1.0f, 1.4f, 0.4f));
  float diff = max(dot(normalize(input.normal), lightDir), 0.2f);

  float3 baseColor = float3(0.1f, 0.1f, 0.4f);
  return float4(baseColor * diff, 1.0f);
}