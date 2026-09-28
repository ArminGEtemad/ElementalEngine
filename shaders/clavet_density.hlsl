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

  float p_health = particles[id].health;
  if (p_health <= 0.0f)
    return;

  float3 p_pos = particles[id].predictedPosition.xyz;

  float density = 0.0f;
  float nearDensity = 0.0f;

  int3 centerCell = getGridCell(p_pos);
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

              float3 r_ij = n_pos - p_pos;
              float dist2 = dot(r_ij, r_ij);

              if (dist2 < particleParams.interactionRadius2) {
                float dist = sqrt(dist2);
                float q = dist / particleParams.interactionRadius;
                float oneMinusQ = 1.0f - q;

                density += (oneMinusQ * oneMinusQ);
                nearDensity += (oneMinusQ * oneMinusQ * oneMinusQ);

                if (n_health < 0.95f) {
                  float heatTransfer = 0.005f * oneMinusQ * particleParams.dt;
                  p_health -= heatTransfer;
                }
              }
            }
          }
          currNeighbor = gridNextBuffer[currNeighbor];
        }
      }
    }
  }

  // burn until nothing is left
  float burnRate = 0.001f;
  if (p_health < 0.95f) {
    p_health -= burnRate * particleParams.dt;
  }

  p_health = max(p_health, 0.0f);

  particles[id].health = p_health;
  particles[id].density = density;
  particles[id].nearDensity = nearDensity;
  particles[id].pressure =
      particleParams.stiffness * (density - particleParams.restDensity);
  particles[id].nearPressure = particleParams.nearStiffness * nearDensity;
}