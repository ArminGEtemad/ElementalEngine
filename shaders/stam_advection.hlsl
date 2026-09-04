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

struct Particle {
  float4 position;
  float4 velocity;
  float4 predictedPosition;

  float density;
  float nearDensity;
  float pressure;
  float nearPressure;
};

[[vk::push_constant]] SimConfigStruct SimConfig;

Texture3D<float> ReadDensity : register(t1);
Texture3D<float4> ReadVelocity : register(t2);
RWTexture3D<float> WriteDensity : register(u3);
RWTexture3D<float4> WriteVelocity : register(u4);
SamplerState LinearSampler : register(s5);
StructuredBuffer<uint> InjectionGrid : register(t6);

[numthreads(8, 8, 8)] void CSMain(uint3 id : SV_DispatchThreadID) {
  if (id.x >= SimConfig.gridWidth || id.y >= SimConfig.gridHeight ||
      id.z >= SimConfig.gridDepth)
    return;

  float3 gridDim =
      float3(SimConfig.gridWidth, SimConfig.gridHeight, SimConfig.gridDepth);
  float3 currentVelocity = ReadVelocity.Load(int4(id, 0)).xyz;

  // trace back using current velocity
  float3 srcPos = (float3)id - (currentVelocity * SimConfig.dt);

  // uv coordinate
  float3 uvw = (srcPos + 0.5f) / gridDim;

  float newDensity = ReadDensity.SampleLevel(LinearSampler, uvw, 0) * 0.9995f;
  float3 newVelocity =
      ReadVelocity.SampleLevel(LinearSampler, uvw, 0).xyz * 0.999995f;

  newVelocity.y += (newDensity * SimConfig.forceY * SimConfig.dt);

  uint flatIndex = id.z * SimConfig.gridWidth * SimConfig.gridHeight +
                   id.y * SimConfig.gridWidth + id.x;
  uint strideIdx = flatIndex * 4;

  // instead of the loop i had before I use the injection method here
  float injectedDensity = asfloat(InjectionGrid[strideIdx + 0]);
  float injectedVelX = asfloat(InjectionGrid[strideIdx + 1]);
  float injectedVelY = asfloat(InjectionGrid[strideIdx + 2]);
  float injectedVelZ = asfloat(InjectionGrid[strideIdx + 3]);

  newDensity += injectedDensity;
  newVelocity += float3(injectedVelX, injectedVelY, injectedVelZ);

  // Cap density to prevent runaway explosions
  float maxDensity = 0.1f;
  newDensity = min(newDensity, maxDensity);

  if (id.x <= 1 || id.x >= SimConfig.gridWidth - 2)
    newVelocity.x = 0.0f;
  if (id.y <= 1 || id.y >= SimConfig.gridHeight - 2)
    newVelocity.y = 0.0f;
  if (id.z <= 1 || id.z >= SimConfig.gridDepth - 2)
    newVelocity.z = 0.0f;

  WriteDensity[id] = newDensity;
  WriteVelocity[id] = float4(newVelocity, 0.0f);
}