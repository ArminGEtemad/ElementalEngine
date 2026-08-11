struct VSOutput {
  float4 position : SV_Position;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD0;
};

float4 FSMain(VSOutput input) : SV_Target {
  float3 vecN = normalize(input.normal);

  // light point
  float3 lightDir = normalize(float3(1.5f, 1.5f, 1.5f));
  float3 lightColor = float3(1.0f, 0.98f, 0.9f); // Warm sunlight

  // lambertian
  float3 ambient = float3(0.05f, 0.05f, 0.08f);
  float NdotL = max(dot(vecN, lightDir), 0.0f);
  float3 diffuse = NdotL * lightColor;

  // Base Terrain
  float3 terrainColor = float3(0.2f, 0.22f, 0.25f);

  float3 finalColor = (ambient + diffuse) * terrainColor;

  return float4(finalColor, 1.0f);
}