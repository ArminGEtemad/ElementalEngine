// this code is based on the algorithm from Clavet Particle-based Viscoelastic
// Fluid Simulation There is a difference here that we visit every cell and not
// as the paper puts for viscosity i < j restriction in Alg 5. Since we visit
// every cell the math is symmetric and there is no need to calculate  vj <- vj
// + I/2 as the paper puts it. It is enough to only calculate
//  vi <- vi - I/2 which indirectly calculates the j too.

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

  // apply gravity
  p.velocity.xyz += GRAVITY * particleParams.dt;

  // apply viscosity
  float3 viscosityImpulse = float3(0, 0, 0);
  int3 centerCell = getGridCell(p.position.xyz);

  // Search 3x3x3 neighbor cells
  for (int z = -1; z <= 1; z++) {
    for (int y = -1; y <= 1; y++) {
      for (int x = -1; x <= 1; x++) {
        uint hash = hashGridCell(centerCell + int3(x, y, z));
        uint currNeighbor = gridHeadBuffer[hash];

        // Traverse the linked list of particles in this cell
        while (currNeighbor != INVALID_ID) {
          // no self interaction
          if (currNeighbor != id) {
            Particle n = particles[currNeighbor];
            float3 r_ij = n.position.xyz - p.position.xyz;
            float dist2 = dot(r_ij, r_ij);

            if (dist2 < particleParams.interactionRadius2 && dist2 > 1e-7f) {
              float dist = sqrt(dist2);
              float3 rHat = r_ij / dist;
              float q = dist / particleParams.interactionRadius;

              // Inward radial velocity
              float u = dot(p.velocity.xyz - n.velocity.xyz, rHat);

              if (u > 0.0f) {
                // apply linear and quadratic impulses
                //  I = dt * (1-q) * (sigma*u + beta*u^2) * rHat
                float impulseMag = particleParams.dt * (1.0f - q) *
                                   (particleParams.linearViscosity * u +
                                    particleParams.quadraticViscosity * u * u);

                // Paper: v_i = v_i - I/2. We accumulate this.
                viscosityImpulse -= (impulseMag * 0.5f) * rHat;
              }
            }
          }
          currNeighbor = gridNextBuffer[currNeighbor]; // Move to next particle
        }
      }
    }
  }

  // Apply accumulated viscosity
  p.velocity.xyz += viscosityImpulse;

  //  advance to predicted position
  p.predictedPosition.xyz = p.position.xyz + p.velocity.xyz * particleParams.dt;

  particles[id] = p;
  // then we move to add and remove springs and change rest lenght in the next
  // step
}