Texture2D HeightmapTex : register(t0);
SamplerState LinearSampler : register(s1);

struct VSOut {
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
};

float4 FSMain(VSOut input) : SV_Target {
  // get the resolution of the heightmap
  float width, height;
  HeightmapTex.GetDimensions(width, height);
  float2 texelSize = float2(1.0f / width, 1.0f / height);

  // sampler the center of the height / health
  float2 hCenterAndHealth = HeightmapTex.Sample(LinearSampler, input.uv).rg;
  float hCenter = hCenterAndHealth.r;

  if (hCenter < 0.15f) {
    discard;
  }

  float avgHealth = saturate(hCenterAndHealth.g / max(hCenter, 0.001f));
  // sample neighbors to calculatee surface derivatives
  float hLeft =
      HeightmapTex.Sample(LinearSampler, input.uv + float2(-texelSize.x, 0.0f))
          .r;
  float hRight =
      HeightmapTex.Sample(LinearSampler, input.uv + float2(texelSize.x, 0.0f))
          .r;
  float hDown =
      HeightmapTex.Sample(LinearSampler, input.uv + float2(0.0f, texelSize.y))
          .r;
  float hUp =
      HeightmapTex.Sample(LinearSampler, input.uv + float2(0.0f, -texelSize.y))
          .r;

  // to make the slime more fake-ish 3D
  float slopeStrength = 50.0f;
  float3 normal = normalize(float3((hLeft - hRight) * slopeStrength,
                                   (hDown - hUp) * slopeStrength, 1.0));

  // volumetric coloring
  // center is darker, thin outer edges are bright translucent green
  float3 baseColor = float3(0.08f, 0.45f, 0.04f);
  float3 deepColor = float3(0.01f, 0.22f, 0.01f);
  float3 slimeColorOrig = lerp(baseColor, deepColor, saturate(hCenter));

  // burning and health based coloring
  float3 burningOrange = float3(1.0f, 0.42f, 0.02f); // Hot ember glow
  float3 charredSoot = float3(0.05f, 0.04f, 0.04f);  // Soot black

  float3 slimeColor;
  if (avgHealth >= 0.95f) {
    slimeColor = slimeColorOrig; // Healthy slime
  } else if (avgHealth > 0.35f) {
    // Active Combustion: Green -> Hot Incandescent Orange
    float t = (avgHealth - 0.35f) / 0.60f;
    slimeColor = lerp(burningOrange, slimeColorOrig, t);
  } else {
    // Dying/Charred: Orange -> Soot Black
    float t = avgHealth / 0.35f;
    slimeColor = lerp(charredSoot, burningOrange, t);
  }

  float3 lightDir =
      normalize(float3(1.0f, 0.8f, 0.3f));   // Light source position
  float3 viewDir = float3(0.0f, 0.0f, 1.0f); // Camera direction
  float3 halfDir = normalize(lightDir + viewDir);

  float diffuse = max(dot(normal, lightDir), 0.0f);
  float specular = pow(max(dot(normal, halfDir), 0.0f),
                       64.0f); // High exponent for wet shine

  // frensel term for more shiny edges
  float fresnel = pow(1.0f - max(dot(normal, viewDir), 0.0f), 4.0f);
  float3 rimColor = float3(0.5f, 0.8f, 1.0f);

  // Combine diffuse, ambient (0.3f to keep it looking translucent), and shiny
  // highlights
  float3 finalColor =
      slimeColor * (diffuse + 0.3f) + float3(0.8f, 1.0f, 0.2f) * specular;
  finalColor += rimColor * (fresnel * 0.1f);

  // Smoothly fade the outer edges of the silhouette
  float alpha = saturate((hCenter - 0.15f) * 4.0f);

  return float4(finalColor, alpha);
}