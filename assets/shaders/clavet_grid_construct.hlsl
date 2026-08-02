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

  uint hash = hashGridCell(getGridCell(particles[id].position));

  uint originalStart;
  InterlockedExchange(gridHeadBuffer[hash], id, originalStart);
  gridNextBuffer[id] = originalStart;
}
