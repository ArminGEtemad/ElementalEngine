struct VSOutput {
  float4 posClip : SV_POSITION;
  float2 uv : TEXCOORD0;
};

float4 FSMain(VSOutput input) : SV_TARGET {
  //  Calculate squared distance from quad center
  float distSq = dot(input.uv, input.uv);

  // sphere shape which will not be looking sphere because Z is not in the
  // calculations
  if (distSq > 1.0) {
    discard;
  }

  // Reconstruct 3D Sphere Surface Normal (Impostor Sphere)
  float z = sqrt(1.0 - distSq);
  float3 normal = normalize(float3(input.uv.x, input.uv.y, z));

  // Simple 3D Directional Light
  float3 lightDir = normalize(float3(0.5, 1.0, 0.4));
  float NdotL = max(0.25, dot(normal, lightDir));

  // Specular Highlight for glossy slime look
  float3 viewDir = float3(0.0, 0.0, 1.0);
  float3 halfVector = normalize(lightDir + viewDir);
  float spec = pow(max(0.0, dot(normal, halfVector)), 32.0);

  // Vibrant Slime Green Color
  float3 slimeColor =
      float3(0.15, 0.85, 0.25) * NdotL + float3(0.8, 1.0, 0.8) * spec;

  return float4(slimeColor, 1.0);
}