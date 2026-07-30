// this code is based on the algorithm from Clavet Particle-based Viscoelastic Fluid Simulation
// There is a difference here that we visit every cell and not as the paper puts for viscosity
// i < j restriction in Alg 5. Since we visit every cell the math is symmetric and there is 
// no need to calculate  vj <- vj + I/2 as the paper puts it. It is enough to only calculate
//  vi <- vi - I/2 which indirectly calculates the j too. 

#include "clavet_common.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> gridHeadBuffer : register(u1);
RWStructuredBuffer<uint> gridNextBuffer : register(u2);
RWStructuredBuffer<Spring> springs : register(u3);

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;
    if (id >= particleParams.numParticles) return;

    Particle p = particles[id];
    
    // apply gravity
    p.velocity += GRAVITY * particleParams.dt;

    // apply lightning strike force
    if (particleParams.lightningOpacity > 0.0001f) {
        float2 strikePos = float2(particleParams.strikeX, particleParams.strikeY);
        float2 toParticle = p.position - strikePos;
        float dist2 = dot(toParticle, toParticle);

        // explosive blast radius 
        float blastRadius = 50.0f; 
        float blastRadius2 = blastRadius * blastRadius;

        if (dist2 < blastRadius2 && dist2 > 1e-4f) {
            float dist = sqrt(dist2);
            float2 forceDir = toParticle / dist; // Radial vector pointing away from impact
            
            // Linear falloff: Maximum force at center, 0 force at the blast edge
            float falloff = 1.0f - (dist / blastRadius);
            
            // Impulse magnitude
            float forceStrength = 1000.0f;
            float impulseMag = forceStrength * falloff * particleParams.lightningOpacity;
            
            // Apply velocity boost outward
            p.velocity += forceDir * impulseMag * particleParams.dt;

            // ignition
            // direct hit by the lightning causes a damage of:
            float ignitionDamage =  falloff * particleParams.lightningOpacity * particleParams.dt;
            p.health -= ignitionDamage;
            p.health = max(p.health, 0.0f);
        }
    }

    // apply viscosity
    float2 viscosityImpulse = float2(0, 0);
    int2 centerCell = getGridCell(p.position);

    // Search 3x3 neighbor cells
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            uint hash = hashGridCell(centerCell + int2(x, y));
            uint currNeighbor = gridHeadBuffer[hash];

            // Traverse the linked list of particles in this cell
            while (currNeighbor != INVALID_ID) {
                // no self interaction
                if (currNeighbor != id) {
                    Particle n = particles[currNeighbor];
                    float2 r_ij = n.position - p.position;
                    float dist2 = dot(r_ij, r_ij);

                    if (dist2 < particleParams.interactionRadius2 && dist2 > 1e-7f) {
                        float dist = sqrt(dist2);
                        float2 rHat = r_ij / dist;
                        float q = dist / particleParams.interactionRadius;
                        
                        // Inward radial velocity
                        float u = dot(p.velocity - n.velocity, rHat);
                        
                        if (u > 0.0f) {
                            // apply linear and quadratic impulses
                            //  I = dt * (1-q) * (sigma*u + beta*u^2) * rHat
                            float impulseMag = particleParams.dt * (1.0f - q) 
                                * (particleParams.linearViscosity * u + particleParams.quadraticViscosity * u * u);
                            
                            // Paper: v_i = v_i - I/2. We accumulate this.
                            viscosityImpulse -= (impulseMag * 0.5f) * rHat;
                        }
                    }
                }
                currNeighbor = gridNextBuffer[currNeighbor]; // Move to next particle
            }
        }
    }

    // Apply accumulated viscosity
    p.velocity += viscosityImpulse;

    //  advance to predicted position
    p.predictedPosition = p.position + p.velocity * particleParams.dt;

    particles[id] = p;
    // then we move to add and remove springs and change rest lenght in the next step
}