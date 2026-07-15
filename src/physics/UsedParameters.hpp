#pragma once

#include <cstdint>

static constexpr uint32_t MAX_SPRINGS = 16;

namespace elementalEngine::Physics {
// Take this configuration structs to another scripts nect to the physics core
// TODO change the name later to match the fact that this is only for the stam
// fluid
struct SimConfig {
  uint32_t gridWidth;
  uint32_t gridHeight;
  float dt;
  float forceY;
  uint32_t numParticles;
  uint32_t pad[3];
};

// paticle based fluid for the acidic slime projectile
struct Particle {
  float position[2];
  float velocity[2];

  float predictedPosition[2];
  float density;
  float nearDensity;

  float pressure;
  float nearPressure;
  uint32_t pad[2];
};

// particle simulation parameters
// Clavet Paper
struct ParticleSimulationParameters {
  float dt;
  uint32_t numParticles;
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
  uint32_t hashGridSize;
};

struct Spring {
  uint32_t neighborID; // INVALID_ID if empty
  float restLength;
};
} // namespace elementalEngine::Physics