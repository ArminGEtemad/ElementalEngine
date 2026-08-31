struct SimConfigStruct {
  uint gridWidth;
  uint gridHeight;
  float dt;
  float forceY;

  uint numParticles;
  float domainDepth;
  float domainWidth;
  float pad;
};

[[vk::push_constant]] SimConfigStruct SimConfig;

Texture2D<float2> ReadVelocity : register(t1);
RWTexture2D<float> WriteDivergence : register(u3);

[numthreads(8, 8, 1)] void CSMain(uint3 id : SV_DispatchThreadID) {
  if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight)
    return;

  // handle boundaries
  uint left = id.x > 0 ? id.x - 1 : id.x;
  uint right = id.x < SimConfig.gridWidth - 1 ? id.x + 1 : id.x;
  uint top = id.y > 0 ? id.y - 1 : id.y;
  uint bottom = id.y < SimConfig.gridHeight - 1 ? id.y + 1 : id.y;

  // velocity is zero at the walls
  float vL = ReadVelocity[uint2(left, id.y)].x;
  float vR = ReadVelocity[uint2(right, id.y)].x;
  float vT = ReadVelocity[uint2(id.x, top)].y;
  float vB = ReadVelocity[uint2(id.x, bottom)].y;

  float divergence = 0.5f * ((vR - vL) + (vB - vT));
  WriteDivergence[id.xy] = divergence;
}