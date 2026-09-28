#include "clavet_common.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> gridHeadBuffer : register(u1);
RWStructuredBuffer<uint> gridNextBuffer : register(u2);
RWStructuredBuffer<Spring> springs : register(u3);

// Build Grid using current Position
[numthreads(THREAD_GROUP_SIZE, 1, 1)] void
CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint id = DTid.x;
  if (id >= particleParams.numParticles)
    return;

  if (particles[id].health <= 0.0f) {
    return;
  }

  float3 pos = particles[id].position.xyz;
  uint hash = hashGridCell(getGridCell(pos));

  uint originalStart;
  InterlockedExchange(gridHeadBuffer[hash], id, originalStart);
  gridNextBuffer[id] = originalStart;
}