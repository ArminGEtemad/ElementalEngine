struct CameraFrameData {
  float4x4 viewMatrix;
  float4x4 projectionMatrix;
  float4x4 viewProjection;
  float4x4 invViewMatrix;
  float4x4 invProjectionMatrix;
  float4x4 invViewProjection;
  float4 cameraPosition;
  float time;
  float deltaTime;
  float2 padding;
};

struct GasRenderConfig {
  float4 domainMin;
  float4 domainMax;
};

[[vk::push_constant]] GasRenderConfig RenderConfig;

StructuredBuffer<uint4> ActiveChunks : register(t0);
Texture3D<float> ReadDensity : register(t1);
SamplerState LinearSampler : register(s2);
Texture2D<float> TerrainDepth : register(t3);
cbuffer FrameData : register(b4) { CameraFrameData camera; };

struct VSOut {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
};

bool IntersectAABB(float3 ro, float3 rd, float3 boxMin, float3 boxMax,
                   out float tNear, out float tFar) {
  float3 invDirection = 1.0f / (rd + 1e-6f);
  float3 t0 = (boxMin - ro) * invDirection;
  float3 t1 = (boxMax - ro) * invDirection;
  float3 tmin = min(t0, t1);
  float3 tmax = max(t0, t1);

  tNear = max(max(tmin.x, tmin.y), tmin.z);
  tFar = min(min(tmax.x, tmax.y), tmax.z);

  return (tNear <= tFar) && (tFar > 0.0f);
}

float4 FSMain(VSOut input) : SV_TARGET {
  float2 ndc = float2(input.uv.x * 2.0f - 1.0f, 1.0f - input.uv.y * 2.0f);
  float4 clipNear = float4(ndc.x, ndc.y, 0.0f, 1.0f);
  float4 clipFar = float4(ndc.x, ndc.y, 1.0f, 1.0f);

  float4 worldNear = mul(camera.invViewProjection, clipNear);
  float4 worldFar = mul(camera.invViewProjection, clipFar);
  worldNear /= worldNear.w;
  worldFar /= worldFar.w;

  float3 ro = camera.cameraPosition.xyz;
  float3 rd = normalize(worldFar.xyz - worldNear.xyz);

  float3 boxMin = RenderConfig.domainMin.xyz;
  float3 boxMax = RenderConfig.domainMax.xyz;

  float tNear, tFar;
  if (!IntersectAABB(ro, rd, boxMin, boxMax, tNear, tFar))
    discard;
  tNear = max(tNear, 0.0f);

  float hwDepth = TerrainDepth.SampleLevel(LinearSampler, input.uv, 0).x;

  if (hwDepth < 1.0f) {
    float4 hitClip = float4(ndc.x, ndc.y, hwDepth, 1.0f);
    float4 hitWorld = mul(camera.invViewProjection, hitClip);
    hitWorld /= hitWorld.w;
    float distToSolid = distance(ro, hitWorld.xyz);
    tFar = min(tFar, distToSolid);
  }

  if (tNear >= tFar)
    discard;

  const int MAX_STEPS = 64;
  float stepSize = (tFar - tNear) / (float)MAX_STEPS;
  float t = tNear;

  float transparency = 1.0f;
  float3 accumColor = float3(0.0f, 0.0f, 0.0f);
  float3 gasColor = float3(0.72f, 0.75f, 0.48f);

  for (int i = 0; i < MAX_STEPS; ++i) {
    if (t >= tFar || transparency <= 0.05f)
      break;

    float3 sampleWorldPos = ro + rd * t;
    float3 uvw = (sampleWorldPos - boxMin) / (boxMax - boxMin);

    if (all(uvw >= 0.0f) && all(uvw <= 1.0f)) {

      uint3 chunkCoord = uint3(uvw * 16.0f);
      chunkCoord = clamp(chunkCoord, uint3(0, 0, 0), uint3(15, 15, 15));
      uint chunkIndex =
          chunkCoord.x + (chunkCoord.y * 16) + (chunkCoord.z * 256);

      // culling
      if (ActiveChunks[chunkIndex].x == 1) {

        float d = ReadDensity.SampleLevel(LinearSampler, uvw, 0);

        // Ignore background noise
        if (d > 0.002f) {
          float density = (d - 0.002f) * 8.0f;
          float stepAbsorption = exp(-density * stepSize * 0.3f);
          float stepAlpha = 1.0f - stepAbsorption;

          accumColor += gasColor * stepAlpha * transparency;
          transparency *= stepAbsorption;
        }
      }
    }

    // Advance ray normally, regardless of whether it was empty or not
    t += stepSize;
  }

  float totalAlpha = 1.0f - transparency;

  // see through smoke
  float3 finalColor = accumColor / max(totalAlpha, 0.0001f);
  return float4(finalColor, totalAlpha);
}
