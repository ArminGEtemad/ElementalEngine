#include "Window.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>

namespace elementalEngine {
// constructing
WindowHandling::WindowHandling(int w, int h, std::string name)
    : width{w}, height{h}, windowName{name} {
  // initalizes the window
  initWindow();
}

// destructing
WindowHandling::~WindowHandling() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

// define functions
void WindowHandling::initWindow() {
  glfwInit();

  // no OpenGL
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // TODO make the window resizable later when the triangle is up
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  // make window
  window =
      glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);

  // error hangling
  if (!window) {
    throw std::runtime_error("Failed to create GLFW Window!");
  }
}
} // namespace elementalEngine