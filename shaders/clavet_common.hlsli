static const uint THREAD_GROUP_SIZE = 256;
static const uint MAX_SPRINGS = 64;
static const uint INVALID_ID = 0xFFFFFFFF;
static const float3 GRID_ORIGIN = float3(0.0, 0.0, 0.0);
static const float3 GRAVITY = float3(0.0f, -100.0f, 0.0f);

struct Particle {
  float4
      position; // 4th state for the AIR and GROUND FLAG for now padding though
  float4 velocity;
  float4 predictedPosition;

  float density;
  float nearDensity;
  float pressure;
  float nearPressure;
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

  float domainWidth;
  float domainDepth;
  float domainHeight;
  float pad;
};

#ifdef __SPIRV__
[[vk::push_constant]] ParticleSimulationParameters particleParams;
#else
ConstantBuffer<ParticleSimulationParameters> particleParams : register(b0);
#endif

// spacial Hashing
int3 getGridCell(float3 pos) {
  return int3(floor((pos - GRID_ORIGIN) / particleParams.cellSpacing));
}

uint hashGridCell(int3 cell) {
  const uint p1 = 73856093;
  const uint p2 = 19349663;
  const uint p3 = 83492791;
  int n = cell.x * p1 ^ cell.y * p2 ^ cell.z * p3;
  n %= (int)particleParams.hashGridSize;
  return (uint)(n < 0 ? n + (int)particleParams.hashGridSize : n);
}