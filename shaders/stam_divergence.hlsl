struct SimConfigStruct {
  uint gridWidth;
  uint gridHeight;
  uint gridDepth;
  float dt;

  float forceY;
  uint numParticles;
  float domainWidth;
  float domainHeight;

  float domainDepth;
  float3 pad;
};

[[vk::push_constant]] SimConfigStruct SimConfig;

Texture3D<float4> ReadVelocity : register(t1);
RWTexture3D<float> WriteDivergence : register(u3);

[numthreads(8, 8, 8)] void CSMain(uint3 id : SV_DispatchThreadID) {
  if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight ||
      id.z >= SimConfig.gridDepth)
    return;

  // handle boundaries
  uint left = id.x > 0 ? id.x - 1 : id.x;
  uint right = id.x < SimConfig.gridWidth - 1 ? id.x + 1 : id.x;
  uint bottom = id.y > 0 ? id.y - 1 : id.y;
  uint top = id.y < SimConfig.gridHeight - 1 ? id.y + 1 : id.y;
  uint back = id.z > 0 ? id.z - 1 : id.z;
  uint front = id.z < SimConfig.gridDepth - 1 ? id.z + 1 : id.z;

  float vL = ReadVelocity.Load(uint4(left, id.y, id.z, 0)).x;
  float vR = ReadVelocity.Load(uint4(right, id.y, id.z, 0)).x;
  float vB = ReadVelocity.Load(uint4(id.x, bottom, id.z, 0)).y;
  float vT = ReadVelocity.Load(uint4(id.x, top, id.z, 0)).y;
  float vBk = ReadVelocity.Load(uint4(id.x, id.y, back, 0)).z;
  float vF = ReadVelocity.Load(uint4(id.x, id.y, front, 0)).z;

  float divergence = 0.5f * ((vR - vL) + (vT - vB) + (vF - vBk));
  WriteDivergence[id] = divergence;
}