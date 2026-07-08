static const float GRAVITI_ACCEL = -9.81;

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

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;

    if (particles[id].state == 1) return; // don't take settled particles into account
    
    float2 pos = particles[id].position;
    float2 vel = particles[id].velocity;

    // in y direction only gravity pulls
    vel.y += GRAVITI_ACCEL * particleParams.dt;
    float2 predPos = pos + vel * particleParams.dt;

    // hardcoded for now Until I make the collisions 
    if (predPos.y < 10.0) {
        vel.y = 0.0;
        predPos.y = 10.0; // stop moving down
        particles[id].state = 1; // settled particles
    }

    particles[id].velocity = vel;
    particles[id].predictedPosition = predPos;
}

