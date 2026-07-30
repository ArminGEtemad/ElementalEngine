static const uint THREAD_GROUP_SIZE = 256;

struct FireParticles {
    float2 position;
    float2 velocity;
    float life;
    float maxLife;
    float temperature; // core 1.0, 0.0 cold (should I transition to smoke?)
    float particleRadius;
};

struct FireSimParameters {
    float dt;
    uint numParticles;
    float buoyancy; // upward thermal lift
    float drag;     // damping factor
    float coolingRate;
    float expansionRate; // hotgas expands particle radius
    float emitterX;      // X position for respawning in GPU
    float emitterY;      // Y position for respawning in GPU
    uint32_t isBurning;
    float3 pad;
};

#ifdef __SPIRV__
[[vk::push_constant]] FireSimParameters particleParams;
#else
ConstantBuffer<FireSimParameters> particleParams : register(b0);
#endif

RWStructuredBuffer<FireParticles> particles : register(u0);

// shader toy  David Hoskins  Hash without Sine
float hash11(float p1) {
    float p = frac(p1 * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}

void respawnParticle(inout FireParticles p, uint id) {
    float seed1 = hash11((float)id * 0.13f);
    float seed2 = hash11((float)id * 0.71f);
    float seed3 = hash11((float)id * 1.17f);
    float seed4 = hash11((float)id * 2.41f);

    // Position spread X
    p.position.x = particleParams.emitterX + (seed1 * 2.0f - 1.0f) * 45.0f;
    p.position.y = particleParams.emitterY;

    // Velocity X in
    p.velocity.x = (seed2 * 2.0f - 1.0f) * 30.0f;
    
    // Upward Velocity Y
    p.velocity.y = 20.0f + seed3 * 90.0f;

    // Lifespan
    float lifetime = 0.8f + seed4 * 2.0f;
    p.life = lifetime;
    p.maxLife = lifetime;

    p.temperature = 1.0f; // Hot core
    p.particleRadius = 4.0f; // Initial radius
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;
    if (id >= particleParams.numParticles) return;

    FireParticles p = particles[id];

     if (p.life <= 0.0f) {
        if (particleParams.isBurning != 0) {
            respawnParticle(p, id);
        } else {
            return; // Stay dead if fire is turned off TODO future updates
        }
    }

    // temperture change with cooling rate
    p.life -= particleParams.dt;
    p.temperature -= particleParams.coolingRate * particleParams.dt;
    p.temperature = max(p.temperature, 0.0f);

    // gas expansion due to temperature 
    p.particleRadius += particleParams.expansionRate * particleParams.dt * p.temperature;

    // air is damping the velocity
    p.velocity *= max(0.0f, 1.0f - (particleParams.drag * p.temperature * particleParams.dt));

    // hat particles move up and changes the velocity in y direction 
    float2 buoyancyForce = float2(0.0f, particleParams.buoyancy *  p.temperature);
    p.velocity += buoyancyForce * particleParams.dt;

    // position 
    p.position += p.velocity * particleParams.dt;

    particles[id] = p;
}