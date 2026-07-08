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
    // This is a 2D version just make sure to change 
    // the scaling factor when moving to 3D
    return (4.0 / (PI * h8)) * diff * diff * diff;
}

float2 SpikyKernel(float2 diffVec, float dist, float h) {
    if (dist <= 0.0001 || dist >= h) return float2(0.0, 0.0);
    float diff = h - dist;
    float h2 = h * h;
    float h4 = h2 * h2;
    float h5 = h4 * h;
    // not sure about the prefactors for 2D but it is just a scaling issue
    float scalar = (-30.0 / (PI * h5)) * diff * diff / dist;

    return diffVec * scalar;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;
     if (id >= particleParams.numParticles) return;
    float2 pos_i = particles[id].predictedPosition;
    float h = particleParams.smoothingRadius;
    int2 gridCoord = int2((pos_i - GRID_ORIGIN) / h);
    // ----------------------------------
    // initializing
    float density = 0.0;
    float2 grad_i = float2(0.0, 0.0);
    float sum_gradient2 = 0.0;
    // ----------------------------------

    // look for 9 neighboring cells
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            int2 cell = gridCoord + int2(x, y);
            if (cell.x < 0 || cell.x >= GRID_WIDTH 
                || cell.y < 0 || cell.y >= GRID_HEIGHT) continue;
            // 1d buffer
            uint neighborID = gridHead[cell.y * GRID_WIDTH + cell.x];

            // until all the addresses are read
            while (neighborID != 0xFFFFFFFF) {
                float2 pos_j = particles[neighborID].predictedPosition;
                float2 r_vec = pos_i - pos_j;
                float dist2 = dot(r_vec, r_vec);

                density += particleParams.particleMass * Poly6(dist2, h);

                if (neighborID != id) {
                    float dist = sqrt(dist2);
                    float2 SpikyGrad = SpikyKernel(r_vec, dist, h);
                    float2 grad_j = -SpikyGrad / particleParams.restDensity;
                    sum_gradient2 += dot(grad_j, grad_j);

                    // accumulate the gradient
                    grad_i -= grad_j;
                }
                neighborID = gridNext[neighborID];
            }
        }
    }
    sum_gradient2 += dot(grad_i, grad_i);
    float constraint = max((density / particleParams.restDensity) - 1.0, 0.0);
    float epsilon = 100.0 / particleParams.stiffness; 
    particles[id].lambda = -constraint / (sum_gradient2 + epsilon);
}

