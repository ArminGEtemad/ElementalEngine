#include "clavet_common.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> gridHeadBuffer : register(u1);
RWStructuredBuffer<uint> gridNextBuffer : register(u2);
RWStructuredBuffer<Spring> springs : register(u3);

[numthreads(THREAD_GROUP_SIZE, 1, 1)] void
CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint id = DTid.x;
  if (id >= particleParams.numParticles)
    return;

  Particle p = particles[id];
  float3 relaxDisplacement = float3(0, 0, 0);

  int3 centerCell = getGridCell(p.predictedPosition.xyz);
  for (int z = -1; z <= 1; z++) {
    for (int y = -1; y <= 1; y++) {
      for (int x = -1; x <= 1; x++) {
        uint hash = hashGridCell(centerCell + int3(x, y, z));
        uint currNeighbor = gridHeadBuffer[hash];

        while (currNeighbor != INVALID_ID) {
          if (currNeighbor != id) {
            Particle n = particles[currNeighbor];

            float3 r_ij = n.predictedPosition.xyz - p.predictedPosition.xyz;
            float dist2 = dot(r_ij, r_ij);

            if (dist2 < particleParams.interactionRadius2 && dist2 > 1e-7f) {
              float dist = sqrt(dist2);
              float q = dist / particleParams.interactionRadius;
              float oneMinusQ = 1.0f - q;
              float3 rHat = r_ij / dist;

              // D = dt^2 * [ P(1-q) + P_near(1-q)^2 ] * rHat
              // this is a lock-free gather, sum BOTH pressures
              float pTerm = (p.pressure + n.pressure) * oneMinusQ;
              float pNearTerm =
                  (p.nearPressure + n.nearPressure) * (oneMinusQ * oneMinusQ);

              float displacementMag =
                  particleParams.dt * particleParams.dt * (pTerm + pNearTerm);

              relaxDisplacement -= (displacementMag * 0.5f) * rHat;
            }
          }

          currNeighbor = gridNextBuffer[currNeighbor];
        }
      }
    }
  }

  // Apply accumulated relaxation
  p.predictedPosition.xyz += relaxDisplacement;
  particles[id] = p;
}