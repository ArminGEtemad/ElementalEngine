// I have added the walls of the screen here too. This is hard coded for now
// just to make sure that the behavior is something I like. I will add obstacles
// and walls separately using SDF and not hardcoded like this.

#include "clavet_common.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);

// Wall friction (0.0 = ice/slip, 1.0 = strict no-slip)
static const float WALL_FRICTION = 0.2f;

[numthreads(THREAD_GROUP_SIZE, 1, 1)] void
CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint id = DTid.x;
  if (id >= particleParams.numParticles)
    return;

  Particle p = particles[id];
  float3 pos = p.predictedPosition.xyz;
  float3 prevPos = p.position.xyz;

  // Use half the interaction radius as the physical collision boundary
  float margin = particleParams.interactionRadius * 0.5f;
  // same equation for stickiness and friction for all the walls

  float halfWidth = particleParams.domainWidth * 0.5f;
  float halfDepth = particleParams.domainDepth * 0.5f;
  float BOUND_Y_MIN = 0.0f;
  float BOUND_Y_MAX = particleParams.domainHeight;
  float BOUND_X_MIN = -halfWidth;
  float BOUND_X_MAX = halfWidth;
  float BOUND_Z_MIN = -halfDepth;
  float BOUND_Z_MAX = halfDepth;

  // Check Floor (Y_MIN)
  float distFloor = pos.y - (BOUND_Y_MIN + margin);
  if (distFloor <= 0.0f) {
    pos.y = BOUND_Y_MIN + margin;

    float3 vel = (pos - prevPos) / particleParams.dt;
    vel.x *= (1.0f - WALL_FRICTION);
    vel.z *= (1.0f - WALL_FRICTION);
    pos.xz = prevPos.xz + vel.xz * particleParams.dt;

    // Cancel the normal velocity component
    prevPos.y = pos.y;
  } else if (distFloor < particleParams.sticknessRadius) {
    // Apply Clavet Stickiness (Equation 12)
    float q = distFloor / particleParams.sticknessRadius;
    float impulse = particleParams.dt * particleParams.sticknessMultiplier *
                    distFloor * (1.0f - q) * (1.0f - q);
    pos.y -= impulse * particleParams.dt; // Pull towards the floor
  }

  // Check Ceiling (Y_MAX)
  float distCeil = (BOUND_Y_MAX - margin) - pos.y;
  if (distCeil <= 0.0f) {
    pos.y = BOUND_Y_MAX - margin;
    float3 vel = (pos - prevPos) / particleParams.dt;
    vel.x *= (1.0f - WALL_FRICTION);
    vel.z *= (1.0f - WALL_FRICTION);
    pos.xz = prevPos.xz + vel.xz * particleParams.dt;
    prevPos.y = pos.y;
  } else if (distCeil < particleParams.sticknessRadius) {
    float q = distCeil / particleParams.sticknessRadius;
    float impulse = particleParams.dt * particleParams.sticknessMultiplier *
                    distCeil * (1.0f - q) * (1.0f - q);
    pos.y += impulse * particleParams.dt;
  }

  // Check Left Wall (X_MIN)
  float distLeft = pos.x - (BOUND_X_MIN + margin);
  if (distLeft <= 0.0f) {
    pos.x = BOUND_X_MIN + margin;
    float3 vel = (pos - prevPos) / particleParams.dt;
    vel.y *= (1.0f - WALL_FRICTION);
    vel.z *= (1.0f - WALL_FRICTION);
    pos.yz = prevPos.yz + vel.yz * particleParams.dt;
    prevPos.x = pos.x;
  } else if (distLeft < particleParams.sticknessRadius) {
    float q = distLeft / particleParams.sticknessRadius;
    float impulse = particleParams.dt * particleParams.sticknessMultiplier *
                    distLeft * (1.0f - q) * (1.0f - q);
    pos.x -= impulse * particleParams.dt;
  }

  // Check Right Wall (X_MAX)
  float distRight = (BOUND_X_MAX - margin) - pos.x;
  if (distRight <= 0.0f) {
    pos.x = BOUND_X_MAX - margin;
    float3 vel = (pos - prevPos) / particleParams.dt;
    vel.y *= (1.0f - WALL_FRICTION);
    vel.z *= (1.0f - WALL_FRICTION);
    pos.yz = prevPos.yz + vel.yz * particleParams.dt;
    prevPos.x = pos.x;
  } else if (distRight < particleParams.sticknessRadius) {
    float q = distRight / particleParams.sticknessRadius;
    float impulse = particleParams.dt * particleParams.sticknessMultiplier *
                    distRight * (1.0f - q) * (1.0f - q);
    pos.x += impulse * particleParams.dt;
  }

  // Check Front Wall (Z_MIN)
  float distFront = pos.z - (BOUND_Z_MIN + margin);
  if (distFront <= 0.0f) {
    pos.z = BOUND_Z_MIN + margin;
    float3 vel = (pos - prevPos) / particleParams.dt;
    vel.x *= (1.0f - WALL_FRICTION);
    vel.y *= (1.0f - WALL_FRICTION);
    pos.xy = prevPos.xy + vel.xy * particleParams.dt;
    prevPos.z = pos.z;
  } else if (distFront < particleParams.sticknessRadius) {
    float q = distFront / particleParams.sticknessRadius;
    float impulse = particleParams.dt * particleParams.sticknessMultiplier *
                    distFront * (1.0f - q) * (1.0f - q);
    pos.z -= impulse * particleParams.dt;
  }

  // Check Back Wall (Z_MAX)
  float distBack = (BOUND_Z_MAX - margin) - pos.z;
  if (distBack <= 0.0f) {
    pos.z = BOUND_Z_MAX - margin;
    float3 vel = (pos - prevPos) / particleParams.dt;
    vel.x *= (1.0f - WALL_FRICTION);
    vel.y *= (1.0f - WALL_FRICTION);
    pos.xy = prevPos.xy + vel.xy * particleParams.dt;
    prevPos.z = pos.z;
  } else if (distBack < particleParams.sticknessRadius) {
    float q = distBack / particleParams.sticknessRadius;
    float impulse = particleParams.dt * particleParams.sticknessMultiplier *
                    distBack * (1.0f - q) * (1.0f - q);
    pos.z += impulse * particleParams.dt;
  }

  // Do the movement
  p.velocity.xyz = (pos - prevPos) / particleParams.dt;
  p.position.xyz = pos;
  p.predictedPosition.xyz = pos;

  particles[id] = p;
}