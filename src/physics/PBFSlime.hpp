#pragma once

#include "Buffer.hpp"
#include "Device.hpp"
#include <cstdint>
#include <memory>
namespace elementalEngine::Physics {

static constexpr uint32_t MAX_SPRINGS = 64;

struct Particle {
  float position[4];          // 4's state is just padding
  float velocity[4];          // 4's state is just padding
  float predictedPosition[4]; //  4's state is just padding
  float density;
  float nearDensity;
  float pressure;
  float nearPressure;
  float health;
  float pad[3];
};

struct ParticleSimulationParameters {
  float dt;
  uint32_t numParticles;
  float interactionRadius;  // h
  float interactionRadius2; // h^2
  float restDensity;        // rho0
  float stiffness;          // k
  float nearStiffness;      // k^near
  float linearViscosity;    // sigma
  float quadraticViscosity; // beta
  float springStiffness;    // k^spring
  float plasticity;         // alpha
  float yieldRatio;         // gamma
  float sticknessRadius;
  float sticknessMultiplier; // mu
  float cellSpacing;
  uint32_t hashGridSize;
  float domainWidth = 2000.0f;
  float domainDepth = 2000.0f;
  float domainHeight = 2000.0f;
  float strikeX;
  float strikeZ;
  float strikeForce;
  float pad[2];
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
  RHI::Buffer *getAliveIndexBuffer() const { return aliveIndexBuffer.get(); }
  RHI::Buffer *getIndirectDrawBuffer() const {
    return indirectDrawBuffer.get();
  }

private:
  RHI::Device &device;
  uint32_t numParticles;
  ParticleSimulationParameters simParams;
  bool isFirstFrame{true};
  std::unique_ptr<RHI::Buffer> aliveIndexBuffer;
  std::unique_ptr<RHI::Buffer> indirectDrawBuffer;
  std::unique_ptr<RHI::Buffer> particleBuffer;
  std::unique_ptr<RHI::Buffer> gridHeadBuffer; // 1 uint per cell
  std::unique_ptr<RHI::Buffer> gridNextBuffer; // 1 uint per particle
  std::unique_ptr<RHI::Buffer> springBuffer;
  std::unique_ptr<RHI::Pipeline> clearGridPipeline;
  std::unique_ptr<RHI::Pipeline> buildGridPipeline;
  std::unique_ptr<RHI::Pipeline> predictionPipeline;
  std::unique_ptr<RHI::Pipeline> buildGridPredictedPipeline;
  std::unique_ptr<RHI::Pipeline> springPipeline;
  std::unique_ptr<RHI::Pipeline> densityPipeline;
  std::unique_ptr<RHI::Pipeline> relaxPipeline;
  std::unique_ptr<RHI::Pipeline> integratePipeline;

  void createPipelines();
  void createRenderPipeline();
  void createResources();
  void initializeParticles();
};
} // namespace elementalEngine::Physics