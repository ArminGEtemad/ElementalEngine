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

Texture3D<float> ReadPressure : register(t1);
Texture3D<float> ReadDivergence : register(t2);
RWTexture3D<float> WritePressure : register(u3);

[numthreads(8, 8, 8)] void CSMain(uint3 id : SV_DispatchThreadID) {
  if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight ||
      id.z >= SimConfig.gridDepth)
    return;

  uint left = id.x > 0 ? id.x - 1 : id.x;
  uint right = id.x < SimConfig.gridWidth - 1 ? id.x + 1 : id.x;
  uint bottom = id.y > 0 ? id.y - 1 : id.y;
  uint top = id.y < SimConfig.gridHeight - 1 ? id.y + 1 : id.y;
  uint back = id.z > 0 ? id.z - 1 : id.z;
  uint front = id.z < SimConfig.gridDepth - 1 ? id.z + 1 : id.z;

  // If neighbor is a wall, use center pressure
  float pL = ReadPressure.Load(uint4(left, id.y, id.z, 0));
  float pR = ReadPressure.Load(uint4(right, id.y, id.z, 0));
  float pB = ReadPressure.Load(uint4(id.x, bottom, id.z, 0));
  float pT = ReadPressure.Load(uint4(id.x, top, id.z, 0));
  float pBk = ReadPressure.Load(uint4(id.x, id.y, back, 0));
  float pF = ReadPressure.Load(uint4(id.x, id.y, front, 0));

  float b = ReadDivergence.Load(uint4(id, 0));

  WritePressure[id] = (pL + pR + pB + pT + pBk + pF - b) / 6.0f;
}