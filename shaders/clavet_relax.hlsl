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

  if (particles[id].health <= 0.0f)
    return;

  float4 p_predictedPos = particles[id].predictedPosition;
  float p_pressure = particles[id].pressure;
  float p_nearPressure = particles[id].nearPressure;

  float3 relaxDisplacement = float3(0, 0, 0);

  int3 centerCell = getGridCell(p_predictedPos.xyz);
  for (int z = -1; z <= 1; z++) {
    for (int y = -1; y <= 1; y++) {
      for (int x = -1; x <= 1; x++) {
        uint hash = hashGridCell(centerCell + int3(x, y, z));
        uint currNeighbor = gridHeadBuffer[hash];

        while (currNeighbor != INVALID_ID) {
          if (currNeighbor != id) {

            float n_health = particles[currNeighbor].health;

            if (n_health > 0.0f) {
              float3 n_pos = particles[currNeighbor].predictedPosition.xyz;

              float3 r_ij = n_pos - p_predictedPos.xyz;
              float dist2 = dot(r_ij, r_ij);

              if (dist2 < particleParams.interactionRadius2 && dist2 > 1e-7f) {

                float n_pressure = particles[currNeighbor].pressure;
                float n_nearPressure = particles[currNeighbor].nearPressure;

                float dist = sqrt(dist2);
                float q = dist / particleParams.interactionRadius;
                float oneMinusQ = 1.0f - q;
                float3 rHat = r_ij / dist;

                float pTerm = (p_pressure + n_pressure) * oneMinusQ;
                float pNearTerm =
                    (p_nearPressure + n_nearPressure) * (oneMinusQ * oneMinusQ);

                float displacementMag =
                    particleParams.dt * particleParams.dt * (pTerm + pNearTerm);

                relaxDisplacement -= (displacementMag * 0.5f) * rHat;
              }
            }
          }
          currNeighbor = gridNextBuffer[currNeighbor];
        }
      }
    }
  }

  p_predictedPos.xyz += relaxDisplacement;
  particles[id].predictedPosition = p_predictedPos;
}