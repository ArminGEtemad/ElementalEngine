#pragma once

#include "Device.hpp"
#include <cstdint>
#include <memory>
namespace elementalEngine::Physics {

static constexpr uint32_t MAX_SPRINGS = 64;

struct Particle {
  float position[4]; // I can use the 4's state for AIR, GROUND flagfor now
                     // padding thoug
  float velocity[4]; // 4's state is just padding
  float predictedPosition[4]; //  4's state is just padding

  float density;
  float nearDensity;
  float pressure;
  float nearPressure;
};

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

  // I used these parameters in my 2D versions so for now I keep them
  float domainWidth = 2000.0f;
  float domainDepth = 2000.0f;
  float domainHeight = 2000.0f;
  float pad;
};

struct Spring {
  uint32_t neighborID; // INVALID_ID if empty
  float restLength;
};

class PBFSlime {
public:
  PBFSlime(RHI::Device &device, uint32_t particleNumberMax);
  ~PBFSlime() = default;

  // called every frame
  void simulate(RHI::CommandList &cmdList, float dt, float strikeX,
                float strikeY, float lightningOpacity);

  RHI::Buffer *getParticleBuffer() const { return particleBuffer.get(); }
  uint32_t getParticleCount() const { return numParticles; }

private:
  RHI::Device &device;
  uint32_t numParticles;
  ParticleSimulationParameters simParams;
  bool isFirstFrame = true;

  // resources
  std::unique_ptr<RHI::Buffer> particleBuffer;
  // Spatial Hashing Linked-List Buffers
  std::unique_ptr<RHI::Buffer> gridHeadBuffer; // 1 uint per cell
  std::unique_ptr<RHI::Buffer> gridNextBuffer; // 1 uint per particle
  std::unique_ptr<RHI::Buffer> springBuffer;

  // compute pipelines
  std::unique_ptr<RHI::Pipeline> clearGridPipeline;
  std::unique_ptr<RHI::Pipeline> buildGridPipeline;
  std::unique_ptr<RHI::Pipeline> predictionPipeline;
  std::unique_ptr<RHI::Pipeline> buildGridPredictedPipeline;
  std::unique_ptr<RHI::Pipeline> springPipeline;
  std::unique_ptr<RHI::Pipeline> densityPipeline;
  std::unique_ptr<RHI::Pipeline> relaxPipeline;
  std::unique_ptr<RHI::Pipeline> integratePipeline;

  // create functions
  void createPipelines();
  void createRenderPipeline();
  void createResources();
  // for the slime it is ok to do on CPU but if I add massive amount of water
  // moving it to GPU
  void initializeParticles(); // cpu initialization (TODO move to GPU)
};
} // namespace elementalEngine::Physics