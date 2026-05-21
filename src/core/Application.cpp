#include "Application.hpp"
#include <GLFW/glfw3.h>

namespace elementalEngine {
Application::Application(){};
Application::~Application(){};

void Application::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
  }
}
} // namespace elementalEngine