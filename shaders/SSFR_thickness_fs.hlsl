struct VSOutput {
  float4 posClip : SV_POSITION;
  float3 posView : TEXCOORD0;
  float2 uv : TEXCOORD1;
};

float4 FSMain(VSOutput input) : SV_Target {
  float distSq = dot(input.uv, input.uv);
  if (distSq > 1.0)
    discard;

  float thickness = sqrt(1.0 - distSq);

  return float4(thickness * 0.02f, 0.0, 0.0, 1.0);
}