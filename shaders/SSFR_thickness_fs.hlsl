struct VSOutput {
  float4 posClip : SV_POSITION;
  float3 posView : TEXCOORD0;
  float2 uv : TEXCOORD1;
  float health : TEXCOORD2;
};

float4 FSMain(VSOutput input) : SV_Target {
  float distSq = dot(input.uv, input.uv);
  if (distSq > 1.0)
    discard;

  float thickness = sqrt(1.0 - distSq);
  float weight = thickness * 0.05;

  // red -> thickness, green -> weighted health

  return float4(weight, weight * input.health, 0.0f, 1.0f);
}