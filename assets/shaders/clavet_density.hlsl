#include "clavet_common.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> gridHeadBuffer : register(u1);
RWStructuredBuffer<uint> gridNextBuffer : register(u2);
RWStructuredBuffer<Spring> springs : register(u3);

// algorithm 2 without relaxation
[numthreads(THREAD_GROUP_SIZE, 1, 1)] void
CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint id = DTid.x;
  if (id >= particleParams.numParticles)
    return;

  Particle p = particles[id];

  float density = 0.0f;
  float nearDensity = 0.0f;

  int2 centerCell = getGridCell(p.predictedPosition);

  for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) {
      uint hash = hashGridCell(centerCell + int2(x, y));
      uint currNeighbor = gridHeadBuffer[hash];

      while (currNeighbor != INVALID_ID) {
        if (currNeighbor != id) {
          Particle n = particles[currNeighbor];

          float2 r_ij = n.predictedPosition - p.predictedPosition;
          float dist2 = dot(r_ij, r_ij);

          if (dist2 < particleParams.interactionRadius2) {
            float dist = sqrt(dist2);
            float q = dist / particleParams.interactionRadius;
            float oneMinusQ = 1.0f - q;

            // rho += (1 - q)^2
            density += (oneMinusQ * oneMinusQ);

            // rho_near += (1 - q)^3
            nearDensity += (oneMinusQ * oneMinusQ * oneMinusQ);

            // heat transfare after lightning strike
            // the first strike drops the health under 0.95
            if (n.health < 0.95 && n.health > 0.0f) {
              float heatTransfer = 0.005 * oneMinusQ * particleParams.dt;
              p.health -= heatTransfer;
            }
          }
        }
        currNeighbor = gridNextBuffer[currNeighbor];
      }
    }
  }

  // burn until nothing is left
  float burnRate = 0.001f;
  if (p.health < 0.95 && p.health > 0.0f) {
    p.health -= burnRate * particleParams.dt;
  }

  p.health = max(p.health, 0.0f);

  // Compute Pressures
  p.density = density;
  p.nearDensity = nearDensity;
  p.pressure =
      particleParams.stiffness * (density - particleParams.restDensity);
  p.nearPressure = particleParams.nearStiffness * nearDensity;

  particles[id] = p;
}