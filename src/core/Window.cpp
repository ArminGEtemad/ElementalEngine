#include "Window.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>
#include <string_view>

namespace elementalEngine {
// constructing
WindowHandling::WindowHandling(int w, int h, std::string_view name)
    : width{w}, height{h} {
  // initalizes the window
  initWindow(name);
}

// destructing
WindowHandling::~WindowHandling() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void WindowHandling::framebufferResizeCallback(GLFWwindow *window, int width,
                                               int height) {
  auto app =
      reinterpret_cast<WindowHandling *>(glfwGetWindowUserPointer(window));
  if (app) {
    app->framebufferResized = true;
  }
}

bool WindowHandling::isMinimized() const {
  int width{0};
  int height{0};
  glfwGetFramebufferSize(window, &width, &height);
  return width == 0 || height == 0;
}

// define functions
void WindowHandling::initWindow(std::string_view name) {
  glfwInit();

  // no OpenGL
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  // make window
  window = glfwCreateWindow(width, height, std::string(name).c_str(), nullptr,
                            nullptr);

  // error handling
  if (!window) {
    throw std::runtime_error("Failed to create GLFW Window!");
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}
} // namespace elementalEngine