#include "clavet_common.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> gridHeadBuffer : register(u1);
RWStructuredBuffer<uint> gridNextBuffer : register(u2);
RWStructuredBuffer<Spring> springs : register(u3);

[numthreads(THREAD_GROUP_SIZE, 1, 1)] 
void CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint id = DTid.x;
  if (id >= particleParams.numParticles)
    return;

  if (particles[id].health <= 0.0f) {
    return;
  }

  float3 predPos = particles[id].predictedPosition.xyz;
  uint hash = hashGridCell(getGridCell(predPos));

  uint originalStart;
  InterlockedExchange(gridHeadBuffer[hash], id, originalStart);
  gridNextBuffer[id] = originalStart;
}