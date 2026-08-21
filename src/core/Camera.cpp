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

void Camera::orbitYaw(float angleRadians) {
  // Delta rotation around world Y-axis (0, 1, 0)
  glm::quat deltaYaw =
      glm::angleAxis(angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));

  // Rotate both position vector and orientation quaternion
  position = deltaYaw * position;
  orientation = glm::normalize(deltaYaw * orientation);
  isDirty = true;
}

void Camera::orbitPitch(float angleRadians) {
  // Delta rotation around local Camera Right vector
  glm::quat deltaPitch = glm::angleAxis(angleRadians, getRightVector());

  // Rotate both position vector and orientation quaternion
  position = deltaPitch * position;
  orientation = glm::normalize(deltaPitch * orientation);
  isDirty = true;
}

void Camera::zoom(float deltaDistance) {
  // Translate along camera forward vector (positive delta moves forward,
  // negative moves backward)
  position += getForwardVector() * deltaDistance;
  isDirty = true;
}

void Camera::orbit(float deltaYaw, float deltaPitch, float deltaZoom) {
  if (deltaYaw != 0.0f)
    orbitYaw(deltaYaw);
  if (deltaPitch != 0.0f)
    orbitPitch(deltaPitch);
  if (deltaZoom != 0.0f)
    zoom(deltaZoom);
}

void Camera::processKeyboardInput(GLFWwindow *window, float deltaTime) {
  float rotateSpeed = 1.5f * deltaTime;
  float zoomSpeed = 15.0f * deltaTime;

  float deltaYaw = 0.0f;
  float deltaPitch = 0.0f;
  float deltaZoom = 0.0f;

  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    deltaYaw -= rotateSpeed;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    deltaYaw += rotateSpeed;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    deltaPitch -= rotateSpeed;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    deltaPitch += rotateSpeed;
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    deltaZoom += zoomSpeed; // Zoom In
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    deltaZoom -= zoomSpeed; // Zoom Out

  if (deltaYaw != 0.0f || deltaPitch != 0.0f || deltaZoom != 0.0f) {
    orbit(deltaYaw, deltaPitch, deltaZoom);
  }
}

} // namespace elementalEngine::Core