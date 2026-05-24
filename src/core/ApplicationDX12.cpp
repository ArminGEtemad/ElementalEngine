#include "ApplicationDX12.hpp"
#include <GLFW/glfw3.h>

namespace elementalEngine {
ApplicationDX12::ApplicationDX12() { initDX12(); }
ApplicationDX12::~ApplicationDX12() {
  // TODO cleaning up
}

void ApplicationDX12::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
  }
}

void ApplicationDX12::initDX12() {
  // TODO creations
}
} // namespace elementalEngine
