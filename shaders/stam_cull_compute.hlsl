struct SimConfig {
  uint gridWidth;
  uint gridHeight;
  uint gridDepth;
  float dt;
  float forceY;
  uint numParticles;
  float domainWidth;
  float domainHeight;
  float domainDepth;
  float strikeX;
  float strikeZ;
  float strikeForce;
};

[[vk::push_constant]] SimConfig pushData;

Texture3D<float> densityTex : register(t0);
RWStructuredBuffer<uint4> activeChunks : register(u1);

[numthreads(8, 8, 8)] void CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint chunksX = pushData.gridWidth / 8;
  uint chunksY = pushData.gridHeight / 8;
  uint chunksZ = pushData.gridDepth / 8;

  if (DTid.x >= chunksX || DTid.y >= chunksY || DTid.z >= chunksZ)
    return;

  bool hasGas = false;
  uint3 startVoxel = DTid * 8;

  // Scan the 8x8x8 area
  for (uint z = 0; z < 8; ++z) {
    for (uint y = 0; y < 8; ++y) {
      for (uint x = 0; x < 8; ++x) {
        float d = densityTex.Load(int4(startVoxel + uint3(x, y, z), 0));
        if (d > 0.005f) {
          hasGas = true;
          break;
        }
      }
      if (hasGas)
        break;
    }
    if (hasGas)
      break;
  }

  // Calculate the 1D index of this chunk and write to the buffer
  uint chunkIndex = DTid.x + (DTid.y * chunksX) + (DTid.z * chunksX * chunksY);
  activeChunks[chunkIndex] = uint4(hasGas ? 1 : 0, 0, 0, 0);
}