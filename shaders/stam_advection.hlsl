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

Texture2D<float> ReadDensity : register(t1);
Texture2D<float2> ReadVelocity : register(t2);
RWTexture2D<float> WriteDensity : register(u3);
RWTexture2D<float2> WriteVelocity : register(u4);
SamplerState LinearSampler : register(s5);
StructuredBuffer<Particle> particles : register(t6);

[numthreads(8, 8, 1)] void
CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
  uint x = dispatchThreadID.x;
  uint y = dispatchThreadID.y;

  if (x >= SimConfig.gridWidth || y >= SimConfig.gridHeight)
    return;
  uint2 index2D = uint2(x, y);
  float2 currentVelocity = ReadVelocity[index2D];

  // trace back using current velocity
  float srcX = (float)x - (currentVelocity.x * SimConfig.dt);
  float srcY = (float)y - (currentVelocity.y * SimConfig.dt);

  // uv coordinate
  float2 uv = float2((srcX + 0.5f) / (float)SimConfig.gridWidth,
                     (srcY + 0.5f) / (float)SimConfig.gridHeight);

  float newDensity = ReadDensity.SampleLevel(LinearSampler, uv, 0) * 0.9995f;
  float2 newVelocity = ReadVelocity.SampleLevel(LinearSampler, uv, 0) * 0.9995f;

  // External Force
  newVelocity.y += (newDensity * SimConfig.forceY * SimConfig.dt);

  float2 cellPos = float2((float)x, (float)y);

  float maxDensity = 0.1f;
  for (uint i = 0; i < SimConfig.numParticles; i++) {
    // to make sure that the calculation doesn't become expensive only 10% of
    // the particles can emit gas
    if (i % 10 != 0)
      continue;

    Particle p = particles[i];

    float gridPx =
        ((p.position.x / SimConfig.domainWidth) + 0.5f) * SimConfig.gridWidth;
    float gridPy =
        ((p.position.z / SimConfig.domainDepth) + 0.5f) * SimConfig.gridHeight;

    float dist = distance(cellPos, float2(gridPx, gridPy));

    if (dist < 3.5f) {
      newDensity += 0.001f;
      newVelocity.x += p.velocity.x * 0.05f * SimConfig.dt;
      newVelocity.y += p.velocity.z * 0.05f * SimConfig.dt;
    }
    // Outer faint gas trail
    else if (dist < 20.0f) {
      newDensity += 0.00001f;
    }
  }

  newDensity = min(newDensity, maxDensity);

  if (x <= 1 || x >= SimConfig.gridWidth - 2)
    newVelocity.x = 0.0f;
  if (y <= 1 || y >= SimConfig.gridHeight - 2)
    newVelocity.y = 0.0f;

  WriteDensity[index2D] = newDensity;
  WriteVelocity[index2D] = newVelocity;
}