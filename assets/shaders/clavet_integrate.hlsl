// I have added the walls of the screen here too. This is hard coded for now
// just to make sure that the behavior is something I like. I will add obstacles and 
// walls separately using SDF and not hardcoded like this. 

#include "clavet_common.hlsli"

RWStructuredBuffer<Particle> particles : register(u0);

static const float BOUND_X_MIN = 0.0f;
static const float BOUND_X_MAX = 2000.0f;
static const float BOUND_Y_MIN = 0.0f;
static const float BOUND_Y_MAX = 2000.0f;

// Wall friction (0.0 = ice/slip, 1.0 = strict no-slip)
static const float WALL_FRICTION = 0.2f; 

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;
    if (id >= particleParams.numParticles) return;

    Particle p = particles[id];
    float2 pos = p.predictedPosition;
    float2 prevPos = p.position;

    // Use half the interaction radius as the physical collision boundary
    float margin = particleParams.interactionRadius * 0.5f;
    // same equation for stickiness and friction for all the walls
    
    // Check Floor (Y_MIN)
    float distFloor = pos.y - (BOUND_Y_MIN + margin);
    if (distFloor <= 0.0f) {
        pos.y = BOUND_Y_MIN + margin;
        
        float2 vel = (pos - prevPos) / particleParams.dt;
        vel.x *= (1.0f - WALL_FRICTION);
        pos.x = prevPos.x + vel.x * particleParams.dt;

        // Cancel the normal velocity component
        prevPos.y = pos.y; 
    } else if (distFloor < particleParams.sticknessRadius) {
        // Apply Clavet Stickiness (Equation 12)
        float q = distFloor / particleParams.sticknessRadius;
        float impulse = particleParams.dt * particleParams.sticknessMultiplier * distFloor * (1.0f - q) * (1.0f - q);
        pos.y -= impulse * particleParams.dt; // Pull towards the floor
    }

    // Check Ceiling (Y_MAX)
    float distCeil = (BOUND_Y_MAX - margin) - pos.y;
    if (distCeil <= 0.0f) {
        pos.y = BOUND_Y_MAX - margin;
        float2 vel = (pos - prevPos) / particleParams.dt;
        vel.x *= (1.0f - WALL_FRICTION);
        pos.x = prevPos.x + vel.x * particleParams.dt;
        prevPos.y = pos.y; 
    } else if (distCeil < particleParams.sticknessRadius) {
        float q = distCeil / particleParams.sticknessRadius;
        float impulse = particleParams.dt * particleParams.sticknessMultiplier * distCeil * (1.0f - q) * (1.0f - q);
        pos.y += impulse * particleParams.dt;
    }

    // Check Left Wall (X_MIN)
    float distLeft = pos.x - (BOUND_X_MIN + margin);
    if (distLeft <= 0.0f) {
        pos.x = BOUND_X_MIN + margin;
        float2 vel = (pos - prevPos) / particleParams.dt;
        vel.y *= (1.0f - WALL_FRICTION);
        pos.y = prevPos.y + vel.y * particleParams.dt;
        prevPos.x = pos.x; 
    } else if (distLeft < particleParams.sticknessRadius) {
        float q = distLeft / particleParams.sticknessRadius;
        float impulse = particleParams.dt * particleParams.sticknessMultiplier * distLeft * (1.0f - q) * (1.0f - q);
        pos.x -= impulse * particleParams.dt;
    }

    // Check Right Wall (X_MAX)
    float distRight = (BOUND_X_MAX - margin) - pos.x;
    if (distRight <= 0.0f) {
        pos.x = BOUND_X_MAX - margin;
        float2 vel = (pos - prevPos) / particleParams.dt;
        vel.y *= (1.0f - WALL_FRICTION);
        pos.y = prevPos.y + vel.y * particleParams.dt;
        prevPos.x = pos.x; 
    } else if (distRight < particleParams.sticknessRadius) {
        float q = distRight / particleParams.sticknessRadius;
        float impulse = particleParams.dt * particleParams.sticknessMultiplier * distRight * (1.0f - q) * (1.0f - q);
        pos.x += impulse * particleParams.dt;
    }

    // deactivate fully consumed slime particles 
    if (p.health <= 0.0f) {
        p.position = float2(-9999.0f, -9999.0f);
        p.predictedPosition = float2(-9999.0f, -9999.0f);
        p.velocity = float2(0.0f, 0.0f);
        particles[id] = p;
        return;
    }

    // Do the movement
    p.velocity = (pos - prevPos) / particleParams.dt;
    p.position = pos;
    p.predictedPosition = pos;

    particles[id] = p;
}