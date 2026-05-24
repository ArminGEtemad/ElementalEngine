#pragma once
#define GLFW_INCLUDE_VULKAN

// add libraries
#include <GLFW/glfw3.h>
#include <string>

namespace elementalEngine {
class WindowHandling {
public:
  WindowHandling(int w, int h, std::string name); // width, height, name
  ~WindowHandling();

  // cleaning up
  WindowHandling(const WindowHandling &) = delete;
  WindowHandling &operator=(const WindowHandling &) = delete;

  // getters and public functions
  bool shouldClose() { return glfwWindowShouldClose(window); }
  GLFWwindow *getGLFWwindow() { return window; }

private:
  // declare variables
  GLFWwindow *window;
  const int width;
  const int height;
  std::string windowName;

  // declare functions
  void initWindow();
};
} // namespace elementalEngine