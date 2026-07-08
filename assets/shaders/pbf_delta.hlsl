static const float PI = 3.14159265359;
static const int GRID_WIDTH = 128;
static const int GRID_HEIGHT = 128;
static const float2 GRID_ORIGIN = float2(-20.0, -20.0);

struct Particle {
    float2 position;
    float2 velocity;
    float2 predictedPosition;
    uint state; // 0: flying 1: settled
    float lambda;
};

struct ParticleSimulationParameters {
    float dt;
    float restDensity;
    float stiffness;
    float viscosity;
    float particleMass;
    float smoothingRadius;
    uint numParticles; // Added to prevent out-of-bounds execution
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

float2 SpikyKernel(float2 diffVec, float dist, float h) {
    if (dist <= 0.0001 || dist >= h) return float2(0.0, 0.0);
    float diff = h - dist;
    float h2 = h * h;
    float h4 = h2 * h2;
    float h5 = h4 * h;
    float scalar = (-30.0 / (PI * h5)) * diff * diff / dist;

    return diffVec * scalar;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;
    
    // Safety check: Avoid execution on inactive/excess threads
    if (id >= particleParams.numParticles) return;
    if (particles[id].state == 1) return;

    float2 pos_i = particles[id].predictedPosition;
    float lambda_i = particles[id].lambda; // Corrected type from float2 to float
    float h = particleParams.smoothingRadius;
    int2 gridCoord = int2((pos_i - GRID_ORIGIN) / h);
    float2 delta_p = float2(0.0, 0.0);

    float k = 0.1; // strength of surface tension
    float n = 4.0;
    float dq2 = (0.1 * h) * (0.1 * h);
    float W_dq = Poly6(dq2, h);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) { 
            int2 cell = gridCoord + int2(x, y);
            if (cell.x < 0 || cell.x >= GRID_WIDTH 
                || cell.y < 0 || cell.y >= GRID_HEIGHT) continue;
            
            uint neighborID = gridHead[cell.y * GRID_WIDTH + cell.x];
            while(neighborID != 0xFFFFFFFF) {
                // Ensure we don't calculate self-collision updates if not needed
                if (neighborID != id) {
                    float2 pos_j = particles[neighborID].predictedPosition;
                    float2 r_vec = pos_i - pos_j;
                    float dist2 = dot(r_vec, r_vec);

                    if (dist2 < h * h) {
                        float dist = sqrt(dist2);
                        float lambda_j = particles[neighborID].lambda;
                        
                        // Artificial Surface Tension Term
                        float s_corr = -k * pow(Poly6(dist2, h) / W_dq, n);
                        
                        float2 grad = SpikyKernel(r_vec, dist, h);
                        
                        // Equation 12 & 14 from the paper
                        delta_p += (lambda_i + lambda_j + s_corr) * grad;
                    }
                }
                // Moved inside the loop to avoid infinite loop
                neighborID = gridNext[neighborID];
            }
        }
    }
    
    particles[id].predictedPosition += delta_p / particleParams.restDensity;
}