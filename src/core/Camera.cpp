#include "Camera.hpp"

namespace elementalEngine::Core {
Camera::Camera(float fovDegrees, float aspectRatio, float nearPlane,
               float farPlane)
    : fov(glm::radians(fovDegrees)), aspect(aspectRatio), zNear(nearPlane),
      zFar(farPlane) {
  updateMatrices();
}

void Camera::rotatePitchYaw(float pitchRadians, float yawRadians) {
  // local pitch and world yaw
  glm::quat pitchQuat =
      glm::angleAxis(pitchRadians, glm::vec3(1.0f, 0.0f, 0.0f)); // Local X
  glm::quat yawQuat =
      glm::angleAxis(yawRadians, glm::vec3(0.0f, 1.0f, 0.0f)); // World Y

  // rotation is (world yaw * current orientation * local pitch)
  orientation = glm::normalize(yawQuat * orientation * pitchQuat);
  isDirty = true;
}

void Camera::setAspectRatio(float width, float height) {
  if (height > 0.0f) {
    aspect = width / height;
    isDirty = true;
  }
}

void Camera::update(float deltaTime, float totalTime) {
  frameData.time = totalTime;
  frameData.deltaTime = deltaTime;

  if (isDirty) {
    updateMatrices();
    isDirty = false;
  }
}

void Camera::updateMatrices() {
  // view matrix: conjugate of quaternion gives inverse rotation
  glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(orientation));
  glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -position);

  frameData.viewMatrix = rotationMatrix * translationMatrix;

  // Perspective Matrix (zero-to-one depth enforced by macro)
  frameData.projectionMatrix = glm::perspective(fov, aspect, zNear, zFar);

  // combine View-Projection
  frameData.viewProjection = frameData.projectionMatrix * frameData.viewMatrix;
  frameData.cameraPosition = glm::vec4(position, 1.0f);
}

} // namespace elementalEngine::Core