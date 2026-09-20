static const uint THREAD_GROUP_SIZE = 256;

struct FireParticles {
  float4 position;
  float4 velocity;

  float life;
  float maxLife;
  float temperature; // core 1.0, 0.0 cold (should I transition to smoke?)
  float particleRadius;
};

struct SlimeParticle {
  float4 position;
  float4 velocity;
  float4 predictedPosition;

  float density;
  float nearDensity;
  float pressure;
  float nearPressure;

  float health;
  float3 pad;
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

  float emitterZ;
  uint isBurning;
  uint slimeParticleCount;
  float time;
};

[[vk::push_constant]] FireSimParameters particleParams;

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
    float seed5 = hash11((float)id * 3.89f);
    float seed6 = hash11((float)id * 4.91f);

    float seedRadius = hash11((float)id * 5.12f);
    float seedExp = hash11((float)id * 6.33f);
    float seedDrag = hash11((float)id * 7.42f);

    // Position spread XZ
    p.position.x = targetSlime.position.x + (seed1 * 2.0f - 1.0f) * 5.0f;
    p.position.y = targetSlime.position.y + 10.0f;
    p.position.z = targetSlime.position.z + (seed5 * 2.0f - 1.0f) * 1.5f;
    p.position.w = 0.5f + (seedDrag * 1.5f);

    // Velocity X Z in
    p.velocity.x =
        targetSlime.velocity.x * 0.5f + (seed2 * 2.0f - 1.0f) * 25.0f;
    p.velocity.z = targetSlime.velocity.z * 0.5f + (seed6 * 2.0f - 1.0f) * 5.0f;

    // Upward Velocity Y
    p.velocity.y = 20.0f + seed3 * 90.0f;

    // flames don't grow at the same speed
    p.velocity.w = 0.3f + (seedExp * 1.2f);

    // Lifespan
    float lifetime = 0.8f + seed4 * 2.0f;
    p.life = lifetime;
    p.maxLife = lifetime;

    p.temperature = 1.0f;                            // Hot core
    p.particleRadius = 15.0f + (seedRadius * 15.0f); // Initial radius
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
      particles[id] = p;
    } else {
      return; // Stay dead if fire is turned off TODO future updates
    }
  }

  // temperture change with cooling rate
  p.life -= particleParams.dt;
  p.temperature -= particleParams.coolingRate * particleParams.dt;
  p.temperature = max(p.temperature, 0.0f);

  // gas expansion due to temperature
  float personalExpansion = particleParams.expansionRate * p.velocity.w;
  p.particleRadius += personalExpansion * particleParams.dt * p.temperature;

  // air is damping the velocity
  float personalDrag = particleParams.drag * p.position.w;
  p.velocity.xyz *=
      max(0.0f, 1.0f - (personalDrag * p.temperature * particleParams.dt));

  float waveSpeed = 10.0f;
  float waveFreq = 0.2f;
  float windForce = 40.0f;

  float uniqueOffset = p.position.w * 20.0f;
  float timeOffset = particleParams.time * waveSpeed;

  float windX = sin(p.position.y * waveFreq - timeOffset + uniqueOffset) +
                cos(p.position.z * waveFreq + timeOffset);
  float windZ =
      cos(p.position.y * waveFreq * 0.8f - timeOffset * 1.2f + uniqueOffset) +
      sin(p.position.x * waveFreq - timeOffset);

  float turbMultiplier = 1.0f - p.temperature;

  p.velocity.x += windX * windForce * turbMultiplier * particleParams.dt;
  p.velocity.z += windZ * windForce * turbMultiplier * particleParams.dt;

  // hat particles move up and changes the velocity in y direction
  float3 buoyancyForce =
      float3(0.0f, particleParams.buoyancy * p.temperature, 0.0f);
  p.velocity.xyz += buoyancyForce * particleParams.dt;

  // position
  p.position.xyz += p.velocity.xyz * particleParams.dt;

  particles[id] = p;
}