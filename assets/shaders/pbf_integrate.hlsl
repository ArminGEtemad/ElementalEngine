static const float PI = 3.14159265359;
static const int GRID_WIDTH = 128;
static const int GRID_HEIGHT = 128;
static const float2 GRID_ORIGIN = float2(-20.0, -20.0);

struct Particle {
    float2 position;
    float2 velocity;
    float2 predictedPosition;
    uint state; // 0: flying 1: settled (used to transition to the stam fluid)
    float lambda;
};

struct ParticleSimulationParameters {
    float dt;
    float restDensity;
    float stiffness;
    float viscosity;
    float particleMass;
    float smoothingRadius;
    uint numParticles;
};

#ifdef __SPIRV__
[[vk::push_constant]] ParticleSimulationParameters particleParams;
#else
ConstantBuffer<ParticleSimulationParameters> particleParams : register(b0);
#endif

RWStructuredBuffer<Particle> particles : register(u0);
RWStructuredBuffer<uint> gridHead : register(u1);
RWStructuredBuffer<uint> gridNext : register(u2);

// SPH kernels
float Poly6(float distanceSquared, float h) {
    float h2 = h * h;
    float h4 = h2 * h2;
    float h8 = h4 * h4;
    if (distanceSquared >= h2) return 0.0;
    float diff = h2 - distanceSquared;
    return (4.0 / (PI * h8)) * diff * diff * diff;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;
    if (id >= particleParams.numParticles) return;

    float2 pos_i = particles[id].position;
    float2 pred_i = particles[id].predictedPosition;
    float h = particleParams.smoothingRadius;
    float2 new_vel = (pred_i - pos_i) / particleParams.dt;
    int2 gridCoord = int2((pred_i - GRID_ORIGIN) / h);

    // initialization
    float2 viscosity_force = float2(0.0, 0.0);

    // 9 neighbors
    for(int y = -1; y <= 1; y++) {
        for(int x = -1; x <= 1; x++) {
            int2 cell = gridCoord + int2(x, y);
            if(cell.x < 0 || cell.x >= GRID_WIDTH 
               || cell.y < 0 || cell.y >= GRID_HEIGHT) continue;
            
            uint neighborID = gridHead[cell.y * GRID_WIDTH + cell.x];
            
            while (neighborID != 0xFFFFFFFF) {
                if (neighborID != id && particles[neighborID].state == 0) {
                    float2 pos_j = particles[neighborID].predictedPosition;
                    float2 vel_j = particles[neighborID].velocity; 
                    
                    float2 diff = pred_i - pos_j;
                    float dist2 = dot(diff, diff);
                    
                    if(dist2 < h * h) {
                        // Pull velocity towards neighbor's velocity
                        viscosity_force += (vel_j - new_vel) * Poly6(dist2, h);
                    }
                }
                neighborID = gridNext[neighborID];
            }
        }
    }
    new_vel += particleParams.viscosity * viscosity_force;

    // Update the final state
    particles[id].velocity = new_vel;
    particles[id].position = pred_i;
}
