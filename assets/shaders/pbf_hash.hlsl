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

// spatial hashing to only look for neighboring cells and not all the particles
[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;

    if (particles[id].state == 1) return; // when particles are settles don't hash them
    
    float2 pos = particles[id].predictedPosition;

    int2 gridCoord = int2((pos - GRID_ORIGIN) / particleParams.smoothingRadius); 

    // against out of bounds
    gridCoord.x = clamp(gridCoord.x, 0, GRID_WIDTH - 1);
    gridCoord.y = clamp(gridCoord.y, 0, GRID_HEIGHT - 1);

    // 1 d for buffer structure
    uint cellIndex = gridCoord.y * GRID_WIDTH + gridCoord.x;

    // atomic operation for shared memory  
    uint oldHead;
    InterlockedExchange(gridHead[cellIndex], id, oldHead);

    // stor the old head in the next array
    gridNext[id] = oldHead;
}
