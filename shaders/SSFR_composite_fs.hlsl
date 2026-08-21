Texture2D<float> BlurredDepthMap : register(t0);
Texture2D<float> ThicknessMap : register(t1);
SamplerState LinearSampler : register(s2);

struct VSOutput {
  float4 posClip : SV_POSITION;
  float2 uv : TEXCOORD0;
};

struct PSOutput {
  float4 color : SV_Target0;
  float depth : SV_Depth;
};

struct PushConstants {
  float4x4 InvProjMatrix;
  float4x4 InvViewMatrix;
  float4x4 ProjMatrix;
  float4 lightDir;
};

[[vk::push_constant]] PushConstants pc;

// Helper: Reconstructs the 3D View-Space position from the Depth Map
float3 ReconstructViewPos(float2 uv, float viewDepth) {
  float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);

  float4 clipPos = float4(ndc, 0.5f, 1.0f);
  float4 viewRay = mul(pc.InvProjMatrix, clipPos);
  viewRay.xyz /= viewRay.w;

  return viewRay.xyz * (viewDepth / viewRay.z);
}

//  Edge-Aware View Position Fetcher
float3 FetchViewPos(int2 coord, float2 uv, float centerDepth, int2 texDim) {
  // Clamp to prevent reading outside the screen
  coord = clamp(coord, int2(0, 0), texDim - 1);
  float depth = BlurredDepthMap.Load(int3(coord, 0)).x;

  // If the neighbor is the background sky, cap it to the center pixel's depth!
  // This physically prevents infinite slopes at the silhouette edge.
  if (depth < -9000.0f) {
    depth = centerDepth;
  }

  return ReconstructViewPos(uv, depth);
}

PSOutput FSMain(VSOutput input) {
  PSOutput output;

  int2 pixelCoord = int2(input.posClip.xy);

  uint width, height;
  BlurredDepthMap.GetDimensions(width, height);
  int2 texDim = int2(width, height);

  float viewDepth = BlurredDepthMap.Load(int3(pixelCoord, 0)).x;

  if (viewDepth < -9000.0f)
    discard;

  float thickness = ThicknessMap.Sample(LinearSampler, input.uv).x;

  if (thickness < 0.05f)
    discard;

  float3 posView = ReconstructViewPos(input.uv, viewDepth);

  // Write hardware depth
  float4 clipPos = mul(pc.ProjMatrix, float4(posView, 1.0f));
  output.depth = clipPos.z / clipPos.w;

  //  ADAPTIVE FINITE DIFFERENCES (ddx/ddy whhere making the edges have teeth
  //  and in this way there is no teeth or at least the teeth are less obvious
  //  depending on the point of view)

  float2 texelSize = 1.0f / float2(width, height);

  // Fetch 3D positions of the 4 neighbors
  float3 posRight =
      FetchViewPos(pixelCoord + int2(1, 0),
                   input.uv + float2(texelSize.x, 0.0f), viewDepth, texDim);
  float3 posLeft =
      FetchViewPos(pixelCoord + int2(-1, 0),
                   input.uv - float2(texelSize.x, 0.0f), viewDepth, texDim);
  float3 posDown =
      FetchViewPos(pixelCoord + int2(0, 1),
                   input.uv + float2(0.0f, texelSize.y), viewDepth, texDim);
  float3 posUp =
      FetchViewPos(pixelCoord + int2(0, -1),
                   input.uv - float2(0.0f, texelSize.y), viewDepth, texDim);

  // Calculate Forward and Backward slopes
  float3 ddxFwd = posRight - posView;
  float3 ddxRev = posView - posLeft;
  float3 ddyFwd = posDown - posView;
  float3 ddyRev = posView - posUp;

  // Pick the slope that has the SMALLEST Z-change.
  // This completely stops the normals from jumping across depth cliffs
  float3 ddxPos = abs(ddxFwd.z) < abs(ddxRev.z) ? ddxFwd : ddxRev;
  float3 ddyPos = abs(ddyFwd.z) < abs(ddyRev.z) ? ddyFwd : ddyRev;

  // Construct the smooth View-Space Normal
  // actually needs more attention because at first I did ddxPos, ddyPos and the
  // light was beneath so I changed the order and it worked
  float3 normalView = normalize(cross(ddyPos, ddxPos));

  // lighting --------------

  // Rotate to World-Space for lighting
  float3 normalWorld = normalize(mul((float3x3)pc.InvViewMatrix, normalView));

  float3 lightDir = normalize(pc.lightDir.xyz);
  float NdotL = max(dot(normalWorld, lightDir), 0.0f);

  float3 viewDir = normalize(mul((float3x3)pc.InvViewMatrix, -posView));
  float3 halfVector = normalize(lightDir + viewDir);
  float spec = pow(max(0.0f, dot(normalWorld, halfVector)), 256.0f) * 1.5f;

  float3 deepColor = float3(0.8f, 0.8f, 0.1f);
  float3 edgeColor = float3(0.25f, 0.55f, 0.25f);

  float transmittance = exp(-thickness * 2.0f);
  float3 volumeColor = lerp(deepColor, edgeColor, transmittance);

  float3 finalColor = (volumeColor * (NdotL + 0.2f)) + float3(spec, spec, spec);

  // Crisp meniscus edge
  float alpha = smoothstep(0.05f, 0.15f, thickness);

  output.color = float4(finalColor, alpha);
  return output;
}