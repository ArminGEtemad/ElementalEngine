// needed because the first attempt for 3D stam fluid was very slow. Here
// instead of checking every grid cell the particles give us their grid index
// themselves. The only thing to check is the race condition that could become a
// major problem

struct SimConfigStruct {
  uint gridWidth;
  uint gridHeight;
  uint gridDepth;
  float dt;

  float forceY;
  uint numParticles;
  float domainWidth;
  float domainHeight;

  float domainDepth;
  float strikeX;
  float strikeZ;
  float strikeForce;
};

struct Particle {
  float4 position;
  float4 velocity;
  float4 predictedPosition;
  float density;
  float nearDensity;
  float pressure;
  float nearPressure;
};

[[vk::push_constant]] SimConfigStruct SimConfig;

StructuredBuffer<Particle> particles : register(t0);
RWStructuredBuffer<uint> InjectionGrid : register(u1);

// This function was written because no matter what I did the InterlockedAdd
// function did not work So i had to write one for the float type. Even though I
// believe InterlockedAdd should work and it doesn't need any work around but
// for now it is working
void InterlockedAddFloat(uint index, float value) {
  uint expected = InjectionGrid[index];
  uint oldVal;
  while (true) {
    uint newVal = asuint(asfloat(expected) +
                         value); // so that the numeric value is not converted

    // the atomic check! chompare the and if they are equal write the newVal
    // into Injection grid
    InterlockedCompareExchange(InjectionGrid[index], expected, newVal, oldVal);
    if (oldVal == expected)
      break; // no other GPU thread modified that memory address and exits
    expected = oldVal;
  }
}

[numthreads(256, 1, 1)] void CSMain(uint3 id : SV_DispatchThreadID) {
  // lightning strike
  if (id.x == 0 && SimConfig.strikeForce > 0.0f) {
    float gridPx = ((SimConfig.strikeX / SimConfig.domainWidth) + 0.5f) *
                   SimConfig.gridWidth;
    float gridPz = ((SimConfig.strikeZ / SimConfig.domainDepth) + 0.5f) *
                   SimConfig.gridDepth;

    int cx = (int)floor(gridPx);
    int cy = 2; // Floor level
    int cz = (int)floor(gridPz);

    int blastRadius = 10; // Grid cells
    for (int z = -blastRadius; z <= blastRadius; z++) {
      for (int y = -blastRadius; y <= blastRadius; y++) {
        for (int x = -blastRadius; x <= blastRadius; x++) {
          int ix = cx + x;
          int iy = cy + y;
          int iz = cz + z;

          if (ix >= 0 && ix < (int)SimConfig.gridWidth && iy >= 0 &&
              iy < (int)SimConfig.gridHeight && iz >= 0 &&
              iz < (int)SimConfig.gridDepth) {

            float3 cellPos = float3(ix, iy, iz);
            float3 centerPos = float3(gridPx, cy, gridPz);

            float3 diff = cellPos - centerPos;
            float dist = length(diff);

            if (dist < (float)blastRadius && dist > 0.1f) {
              uint flatIndex = iz * SimConfig.gridWidth * SimConfig.gridHeight +
                               iy * SimConfig.gridWidth + ix;
              uint strideIdx = flatIndex * 4;

              float3 outwardDir = diff / dist;

              float normalizedDist = dist / (float)blastRadius;
              // the falloff should be more smooth using this rather the abropt
              // one
              float falloff = (1.0f - normalizedDist) * (1.0f - normalizedDist);

              float force = SimConfig.strikeForce * falloff;

              InterlockedAddFloat(strideIdx + 1,
                                  outwardDir.x * force * SimConfig.dt);
              InterlockedAddFloat(strideIdx + 2,
                                  outwardDir.y * force * SimConfig.dt);
              InterlockedAddFloat(strideIdx + 3,
                                  outwardDir.z * force * SimConfig.dt);
            }
          }
        }
      }
    }
  }

  if (id.x >= SimConfig.numParticles)
    return;

  Particle p = particles[id.x];

  // directly copied from the advection shaders.
  float gridPx =
      ((p.position.x / SimConfig.domainWidth) + 0.5f) * SimConfig.gridWidth;
  float gridPy = (p.position.y / SimConfig.domainHeight) * SimConfig.gridHeight;
  float gridPz =
      ((p.position.z / SimConfig.domainDepth) + 0.5f) * SimConfig.gridDepth;

  int cx = (int)floor(gridPx);
  int cy = (int)floor(gridPy);
  int cz = (int)floor(gridPz);

  // injection into local radius
  int radius = 3;
  for (int z = -radius; z <= radius; z++) {
    for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
        int ix = cx + x;
        int iy = cy + y;
        int iz = cz + z;

        // Bounds check
        if (ix >= 0 && ix < (int)SimConfig.gridWidth && iy >= 0 &&
            iy < (int)SimConfig.gridHeight && iz >= 0 &&
            iz < (int)SimConfig.gridDepth) {
          float dist =
              distance(float3(ix, iy, iz), float3(gridPx, gridPy, gridPz));
          if (dist < 3.0f) {
            uint flatIndex = iz * SimConfig.gridWidth * SimConfig.gridHeight +
                             iy * SimConfig.gridWidth + ix;
            uint strideIdx = flatIndex * 4;

            // Translate World-Space Velocity to Grid-Space Velocity. Before
            // rewriting the gas were moving faster than the slime. So
            float3 cellSize =
                float3(SimConfig.domainWidth / (float)SimConfig.gridWidth,
                       SimConfig.domainHeight / (float)SimConfig.gridHeight,
                       SimConfig.domainDepth / (float)SimConfig.gridDepth);
            float3 gridVel = p.velocity.xyz / cellSize;

            float velScale =
                0.001f; // the gas was outrunning the slim and in this way I can
                        // slow down the gas at least.

            InterlockedAddFloat(strideIdx + 0, 0.0001f); // Density
            InterlockedAddFloat(strideIdx + 1,
                                gridVel.x * velScale * SimConfig.dt);
            InterlockedAddFloat(strideIdx + 2,
                                gridVel.y * velScale * SimConfig.dt);
            InterlockedAddFloat(strideIdx + 3,
                                gridVel.z * velScale * SimConfig.dt);
          }
        }
      }
    }
  }
}