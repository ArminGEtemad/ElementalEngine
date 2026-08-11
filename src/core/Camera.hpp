#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace elementalEngine::Core {

// should be register as b0
struct alignas(16) CameraFrameData {
  glm::mat4 viewMatrix{1.0f};
  glm::mat4 projectionMatrix{1.0f};
  glm::mat4 viewProjection{1.0f};
  glm::vec4 cameraPosition{0.0f};
  float time{0.0f};
  float deltaTime{0.0f};
  float padding[2]{0.0f, 0.0f};
};

class Camera {
public:
  Camera(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);
  ~Camera() = default;

  // movement and orientation
  void setPosition(const glm::vec3 &newPos) {
    position = newPos;
    isDirty = true;
  }
  void translate(const glm::vec3 &deltaPos) {
    position += deltaPos;
    isDirty = true;
  }
  void setOrientation(const glm::quat &newOrient) {
    orientation = glm::normalize(newOrient);
    isDirty = true;
  }

  // rotate relative to current orientation using a delta quaternion
  void rotate(const glm::quat &deltaRotate) {
    orientation = glm::normalize(deltaRotate * orientation);
    isDirty = true;
  }

  void rotatePitchYaw(float pitchRadians, float yawRadians);
  void setAspectRatio(float width, float height);
  void update(float deltaTime, float totalTime);

  // Camera Direction Vectors
  glm::vec3 getForwardVector() const {
    return orientation * glm::vec3(0.0f, 0.0f, -1.0f);
  }
  glm::vec3 getRightVector() const {
    return orientation * glm::vec3(1.0f, 0.0f, 0.0f);
  }
  glm::vec3 getUpVector() const {
    return orientation * glm::vec3(0.0f, 1.0f, 0.0f);
  }

  const CameraFrameData &getFrameData() const { return frameData; }
  const glm::vec3 &getPosition() const { return position; }
  const glm::quat &getOrientation() const { return orientation; }

private:
  void updateMatrices();

  glm::vec3 position{0.0f, 0.0f, 5.0f};
  glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f}; // identity

  float fov{glm::radians(50.0f)};
  float aspect{16.0f / 9.0f};
  float zNear{0.1f};
  float zFar{1000.0f};

  // lazy evaluation flag
  bool isDirty{true};

  CameraFrameData frameData{};
};
} // namespace elementalEngine::Core