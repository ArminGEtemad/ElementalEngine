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

  float4 p_predictedPosFull = particles[id].predictedPosition;
  float3 p_predictedPos = p_predictedPosFull.xyz;

  float3 springDisplacement = float3(0, 0, 0);
  uint springOffset = id * MAX_SPRINGS;

  uint activeSpringCount = 0;
  uint activeNeighbors[MAX_SPRINGS];

  uint maxValidIndex = 0;
  bool hasAnyValid = false;

  // Process existing springs (Plasticity & Elasticity)
  // alg 4 (Lines 6-14) & alg 3
  // reverse the algorithm from the paper because we already have the
  // springs and we cannot add new springs. We should remove them first and then
  // add.
  for (uint i = 0; i < MAX_SPRINGS; i++) {
    uint sIdx = springOffset + i;
    Spring s = springs[sIdx];

    if (s.neighborID != INVALID_ID) {
      maxValidIndex = i;
      hasAnyValid = true;

      float3 n_predictedPos = particles[s.neighborID].predictedPosition.xyz;

      float3 r_ij = n_predictedPos - p_predictedPos;
      float distSq = dot(r_ij, r_ij);
      bool keepSpring = true;

      if (distSq > 1e-7f) {
        float dist = sqrt(distSq);
        float toleDeform = particleParams.yieldRatio * s.restLength;

        if (dist > s.restLength + toleDeform) {
          s.restLength += particleParams.dt * particleParams.plasticity *
                          (dist - s.restLength - toleDeform);
        } else if (dist < s.restLength - toleDeform) {
          s.restLength -= particleParams.dt * particleParams.plasticity *
                          (s.restLength - toleDeform - dist);
        }

        if (s.restLength > particleParams.interactionRadius) {
          keepSpring = false; // Spring breaks
        } else {
          float fade = 1.0f - (s.restLength / particleParams.interactionRadius);
          float displacementMag = particleParams.dt * particleParams.dt *
                                  particleParams.springStiffness * fade *
                                  (s.restLength - dist);

          float3 rHat = r_ij / dist;
          springDisplacement -= (displacementMag * 0.5f) * rHat;
        }
      }

      if (keepSpring) {
        springs[springOffset + activeSpringCount] = s;
        activeNeighbors[activeSpringCount] = s.neighborID;
        activeSpringCount++;
      }
    }
  }

  if (hasAnyValid) {
    for (uint j = activeSpringCount; j <= maxValidIndex; j++) {
      Spring emptySpring;
      emptySpring.neighborID = INVALID_ID;
      emptySpring.restLength = 0.0f;
      springs[springOffset + j] = emptySpring;
    }
  }

  bool isFull = (activeSpringCount >= MAX_SPRINGS);
  // Part 2: Add new springs
  // alg 4 (Lines 1-5)

  if (!isFull) {
    int3 centerCell = getGridCell(p_predictedPos);

    for (int z = -1; z <= 1 && !isFull; z++) {
      for (int y = -1; y <= 1 && !isFull; y++) {
        for (int x = -1; x <= 1 && !isFull; x++) {
          uint hash = hashGridCell(centerCell + int3(x, y, z));
          uint currNeighbor = gridHeadBuffer[hash];

          while (currNeighbor != INVALID_ID && !isFull) {
            if (currNeighbor != id) {

              float3 n_predictedPos =
                  particles[currNeighbor].predictedPosition.xyz;

              float3 r_ij = n_predictedPos - p_predictedPos;
              float dist2 = dot(r_ij, r_ij);

              if (dist2 < particleParams.interactionRadius2 && dist2 > 1e-7f) {

                bool springExists = false;
                for (uint k = 0; k < activeSpringCount; k++) {
                  if (activeNeighbors[k] == currNeighbor) {
                    springExists = true;
                    break;
                  }
                }

                if (!springExists) {
                  float dist = sqrt(dist2);

                  Spring newSpring;
                  newSpring.neighborID = currNeighbor;

                  // ::this is not from the paper::
                  // in the paper the restLength is h at this point but it did
                  // not show the behavior I liked when rendering. using dist
                  // instead of h worked more like the gooey and jelly that I
                  // wanted.
                  newSpring.restLength = dist;

                  springs[springOffset + activeSpringCount] = newSpring;
                  activeNeighbors[activeSpringCount] =
                      currNeighbor; // Keep cache updated
                  activeSpringCount++;

                  if (activeSpringCount >= MAX_SPRINGS) {
                    isFull = true;
                  }
                }
              }
            }
            currNeighbor = gridNextBuffer[currNeighbor];
          }
        }
      }
    }
  }

  // Apply accumulated spring displacements to predicted position
  p_predictedPosFull.xyz = p_predictedPos + springDisplacement;
  particles[id].predictedPosition = p_predictedPosFull;
}