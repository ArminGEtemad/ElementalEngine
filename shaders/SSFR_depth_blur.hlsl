Texture2D<float> InputDepth : register(t0);
RWTexture2D<float> OutputDepth : register(u1);

struct PushConstants {
  int2 blurDir;       // (1, 0) for Horizontal, (0, 1) for Vertical
  float filterRadius; // How far to blur
  float spatialScale; // The blur curve
  float depthScale;   // How strictly it preserves sharp depth edges
  float3 pad;
};

[[vk::push_constant]] PushConstants pc;

[numthreads(16, 16, 1)] void CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint width, height;
  InputDepth.GetDimensions(width, height);
  if (DTid.x >= width || DTid.y >= height)
    return;

  int2 centerCoord = int2(DTid.xy);
  float centerDepth = InputDepth.Load(int3(centerCoord, 0));

  // If there's no fluid here (we cleared the background)
  // no need for blur
  if (centerDepth < -9000.0f) {
    OutputDepth[centerCoord] = centerDepth;
    return;
  }

  float sum = 0.0f;
  float weightSum = 0.0f;

  int radius = (int)pc.filterRadius;

  for (int i = -radius; i <= radius; i++) {
    int2 sampleCoord = centerCoord + pc.blurDir * i;

    // Clamp to screen edges
    sampleCoord = clamp(sampleCoord, int2(0, 0), int2(width - 1, height - 1));

    float sampleDepth = InputDepth.Load(int3(sampleCoord, 0));

    // If the neighbor is empty sky, ignore it
    if (sampleDepth < -9000.0f)
      continue;

    // Spatial Weight (pixels further away horizontally/vertically have less
    // impact)
    float spatialWeight =
        exp(-(float(i * i) / (2.0f * pc.spatialScale * pc.spatialScale)));

    // Depth Weight (pixels that are drastically deeper/closer have almost zero
    // impact)
    float depthDiff = centerDepth - sampleDepth;
    float depthWeight =
        exp(-(depthDiff * depthDiff) / (2.0f * pc.depthScale * pc.depthScale));

    float weight = spatialWeight * depthWeight;
    sum += sampleDepth * weight;
    weightSum += weight;
  }

  // Output the melted depth!
  if (weightSum > 0.0f) {
    OutputDepth[centerCoord] = sum / weightSum;
  } else {
    OutputDepth[centerCoord] = centerDepth;
  }
}