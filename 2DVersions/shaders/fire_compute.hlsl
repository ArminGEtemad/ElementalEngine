static const uint THREAD_GROUP_SIZE = 256;

struct FireParticles {
  float2 position;
  float2 velocity;
  float life;
  float maxLife;
  float temperature; // core 1.0, 0.0 cold (should I transition to smoke?)
  float particleRadius;
};

struct SlimeParticle {
  float2 position;
  float2 velocity;
  float2 predictedPosition;
  float density;
  float nearDensity;
  float pressure;
  float nearPressure;

  float health;
  float pad;
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
  uint isBurning;
  uint slimeParticleCount;
  float2 pad;
};

#ifdef __SPIRV__
[[vk::push_constant]] FireSimParameters particleParams;
#else
ConstantBuffer<FireSimParameters> particleParams : register(b0);
#endif

RWStructuredBuffer<FireParticles> particles : register(u0);

StructuredBuffer<SlimeParticle> slimeParticles : register(t1);

// shader toy  David Hoskins  Hash without Sine
float hash11(float p1) {
  float p = frac(p1 * 0.1031);
  p *= p + 33.33;
  p *= p + p;
  return frac(p);
}

void respawnParticle(inout FireParticles p, uint id) {
  if (particleParams.slimeParticleCount == 0) {
    p.life = 0.0f;
    return;
  }
  // fire particles randomly distribute themeselves over burning particles
  float seedHash = hash11((float)id * 0.31f);
  uint targetSlimeIdx =
      uint(seedHash * (float)particleParams.slimeParticleCount) %
      particleParams.slimeParticleCount;

  SlimeParticle targetSlime = slimeParticles[targetSlimeIdx];

  if (targetSlime.health <= 0.8f && targetSlime.health > 0.0f) {

    float seed1 = hash11((float)id * 0.13f);
    float seed2 = hash11((float)id * 0.71f);
    float seed3 = hash11((float)id * 1.17f);
    float seed4 = hash11((float)id * 2.41f);

    // Position spread X
    p.position.x = targetSlime.position.x + (seed1 * 2.0f - 1.0f) * 5.0f;
    p.position.y = targetSlime.position.y;

    // Velocity X in
    p.velocity.x =
        targetSlime.velocity.x * 0.5f + (seed2 * 2.0f - 1.0f) * 25.0f;

    // Upward Velocity Y
    p.velocity.y = 20.0f + seed3 * 90.0f;

    // Lifespan
    float lifetime = 0.8f + seed4 * 2.0f;
    p.life = lifetime;
    p.maxLife = lifetime;

    p.temperature = 1.0f;    // Hot core
    p.particleRadius = 2.0f; // Initial radius
  } else {
    // if the slime particle is dead so is the fire particle
    p.life = 0.0;
  }
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)] void
CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint id = DTid.x;
  if (id >= particleParams.numParticles)
    return;

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
  p.particleRadius +=
      particleParams.expansionRate * particleParams.dt * p.temperature;

  // air is damping the velocity
  p.velocity *= max(
      0.0f, 1.0f - (particleParams.drag * p.temperature * particleParams.dt));

  // hat particles move up and changes the velocity in y direction
  float2 buoyancyForce = float2(0.0f, particleParams.buoyancy * p.temperature);
  p.velocity += buoyancyForce * particleParams.dt;

  // position
  p.position += p.velocity * particleParams.dt;

  particles[id] = p;
}