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

  // interactivity with the lightining strike
  float strikeX;
  float strikeY;
  float lightningOpacity;
  float pad;
};

struct Spring {
  uint32_t neighborID; // INVALID_ID if empty
  float restLength;
};

// everything related to the fire particles
struct FireParticles {
  float position[2];
  float velocity[2];

  float life;
  float maxLife;
  float temperature; // core 1.0, 0.0 cold (should I transition to smoke?)
  float particleRadius;
};

struct FireSimParameters {
  float dt;
  uint32_t numParticles;
  float buoyancy; // upward thermal lift
  float drag;     // damping factor

  float coolingRate;
  float expansionRate; // hotgas expands particle radius
  float pad[2];
};

} // namespace elementalEngine::Physics