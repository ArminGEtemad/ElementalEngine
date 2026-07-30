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
  float2 pad;
};

#ifdef __SPIRV__
[[vk::push_constant]] FireSimParameters particleParams;
#else
ConstantBuffer<FireSimParameters> particleParams : register(b0);
#endif

RWStructuredBuffer<FireParticles> particles : register(u0);

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;
    if (id >= particleParams.numParticles) return;

    FireParticles p = particles[id];

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