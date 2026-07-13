static const uint THREAD_GROUP_SIZE = 256;
static const uint MAX_SPRINGS = 16;
static const uint INVALID_ID = 0xFFFFFFFF;
static const float2 GRID_ORIGIN = float2(0.0, 0.0);
static const float2 GRAVITY = float2(0.0f, -9.81f);

struct Particle {
  float2 position;
  float2 velocity;

  float2 predictedPosition;
  float density;
  float nearDensity;

  float pressure;
  float nearPressure;
  uint2 pad;
};

struct Spring {
  uint neighborID;
  float restLength;
};

struct ParticleSimulationParameters {
  float dt;
  uint numParticles;
  float interactionRadius;  // h
  float interactionRadius2; // h^2

  float restDensity;     // rho0
  float stiffness;       // k
  float nearStiffness;   // k^near
  float linearViscosity; // sigma

  float quadraticViscosity; // beta
  float springStiffness;    // k^spring
  float plasticity;         // alpha
  float yieldRatio;         // gamma

  float sticknessRadius;
  float sticknessMultiplier; // mu
  float cellSpacing;
  uint hashGridSize;
};

#ifdef __SPIRV__
[[vk::push_constant]] ParticleSimulationParameters particleParams;
#else
ConstantBuffer<ParticleSimulationParameters> particleParams : register(b0);
#endif

// spacial Hashing
int2 getGridCell(float2 pos) {
    return int2(floor((pos - GRID_ORIGIN) / particleParams.cellSpacing));
}

uint hashGridCell(int2 cell) {
    const uint p1 = 73856093;
    const uint p2 = 19349663;
    int n = cell.x * p1 ^ cell.y * p2;
    n %= (int)particleParams.hashGridSize;
    return (uint)(n < 0 ? n + (int)particleParams.hashGridSize : n);
}