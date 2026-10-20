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

groupshared uint s_hasGas;

[numthreads(8, 8, 8)] void CSMain(uint3 DTid : SV_DispatchThreadID,
                                  uint3 Gid : SV_GroupID,
                                  uint GI : SV_GroupIndex) {
  if (GI == 0) {
    s_hasGas = 0;
  }

  GroupMemoryBarrierWithGroupSync();

  if (DTid.x < pushData.gridWidth && DTid.y < pushData.gridHeight &&
      DTid.z < pushData.gridDepth) {
    float d = densityTex.Load(int4(DTid, 0));
    if (d > 0.005f) {
      InterlockedOr(s_hasGas, 1);
    }
  }

  GroupMemoryBarrierWithGroupSync();

  if (GI == 0) {
    uint chunksX = pushData.gridWidth / 8;
    uint chunksY = pushData.gridHeight / 8;
    uint chunkIndex = Gid.x + (Gid.y * chunksX) + (Gid.z * chunksX * chunksY);
    activeChunks[chunkIndex] = uint4(s_hasGas, 0, 0, 0);
  }
}