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
  float3 springDisplacement = float3(0, 0, 0);
  uint springOffset = id * MAX_SPRINGS;

  // Process existing springs (Plasticity & Elasticity)
  // alg 4 (Lines 6-14) & alg 3
  // reverse the algorithm from the paper because we already have the springs
  // and we cannot add new springs. We should remove them first and then
  // add.
  for (uint i = 0; i < MAX_SPRINGS; i++) {
    uint sIdx = springOffset + i;
    Spring s = springs[sIdx];

    if (s.neighborID != INVALID_ID) {
      Particle n = particles[s.neighborID];

      float3 r_ij = n.predictedPosition.xyz - p.predictedPosition.xyz;
      float distSq = dot(r_ij, r_ij);

      // making sure there is no division by 0
      if (distSq > 1e-7f) {
        float dist = sqrt(distSq);

        // tolerable deformation
        float toleDeform = particleParams.yieldRatio * s.restLength;

        if (dist > s.restLength + toleDeform) {
          // stretch
          s.restLength += particleParams.dt * particleParams.plasticity *
                          (dist - s.restLength - toleDeform);
        } else if (dist < s.restLength - toleDeform) {
          // compress
          s.restLength -= particleParams.dt * particleParams.plasticity *
                          (s.restLength - toleDeform - dist);
        }

        // break spring if rest length exceeds interaction radius
        if (s.restLength > particleParams.interactionRadius) {
          s.neighborID = INVALID_ID;
          springs[sIdx] = s;
          continue;
        }

        // spring displacement alg 3
        float fade = 1.0f - (s.restLength / particleParams.interactionRadius);
        // D = dt^2 * k_spring * (1 - L/h) * (L - r) * rHat
        float displacementMag = particleParams.dt * particleParams.dt *
                                particleParams.springStiffness * fade *
                                (s.restLength - dist);

        float3 rHat = r_ij / dist;

        //  x_i = x_i - D/2
        // and like before no need for the calculation for j since we are doing
        // every cell
        springDisplacement -= (displacementMag * 0.5f) * rHat;

        // Write back updated rest length
        springs[sIdx] = s;
      }
    }
  }

  // Part 2: Add new springs
  // alg 4 (Lines 1-5)
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

              // Check if a spring already exists
              bool springExists = false;
              int emptySlot = -1;

              for (uint k = 0; k < MAX_SPRINGS; k++) {
                uint sIdx = springOffset + k;
                if (springs[sIdx].neighborID == currNeighbor) {
                  springExists = true;
                  break;
                } else if (springs[sIdx].neighborID == INVALID_ID &&
                           emptySlot == -1) {
                  emptySlot = (int)k; // Track the first available empty slot
                }
              }

              // If no spring exists and we have space, add it!
              if (!springExists && emptySlot != -1) {
                Spring newSpring;
                newSpring.neighborID = currNeighbor;
                // ::this is not from the paper::
                // in the paper the restLength is h at this point but it did not
                // show the behavior I liked when rendering. using dist instead
                // of h worked more like the gooey and jelly that I wanted.
                newSpring.restLength = dist;
                // newSpring.restLength = particleParams.interactionRadius;
                springs[springOffset + emptySlot] = newSpring;
              }
            }
          }
          currNeighbor = gridNextBuffer[currNeighbor]; // Traverse linked list
        }
      }
    }
  }

  // Apply accumulated spring displacements to predicted position
  p.predictedPosition.xyz += springDisplacement;
  particles[id] = p;
}