struct VSOutput {
  float4 position : SV_Position;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD0;
  float3 worldPos : TEXCOORD1;
};

struct PushConstants {
  float4 targetPos;
};

[[vk::push_constant]] PushConstants pc;

float4 FSMain(VSOutput input, bool isFrontFace : SV_IsFrontFace) : SV_Target {

  // normal shows downwards when looking the underneath the plane
  float3 actualNormal = isFrontFace ? input.normal : -input.normal;

  float3 vecN = normalize(actualNormal);

  // light point
  float3 lightDir = normalize(float3(1.5f, 1.5f, 1.5f));
  float3 lightColor = float3(1.0f, 0.98f, 0.9f); // Warm sunlight

  // lambertian
  float3 ambient = float3(0.05f, 0.05f, 0.08f);

  // Because the normal points DOWN on the back face, NdotL becomes 0.0
  float NdotL = max(dot(vecN, lightDir), 0.0f);
  float3 diffuse = NdotL * lightColor;

  // Base Terrain
  float3 terrainColor = float3(0.2f, 0.22f, 0.25f);

  // Back face will now only receive ambient * terrainColor
  float3 finalColor = (ambient + diffuse) * terrainColor;
  // ===== make target for zapping
  float distToTarget = distance(input.worldPos.xz, pc.targetPos.xz);

  // ring with a radius of 1.0 and a thickness of 0.1
  float ring = smoothstep(0.9f, 1.0f, distToTarget) -
               smoothstep(1.0f, 1.1f, distToTarget);

  //  center dot
  float dot = 1.0f - smoothstep(0.1f, 0.2f, distToTarget);

  float3 targetColor = float3(0.1f, 0.8f, 1.0f);

  // Additive blend the glowing UI over the terrain
  finalColor += targetColor * max(ring, dot);

  return float4(finalColor, 1.0f);
}