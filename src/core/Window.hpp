#pragma once
#define GLFW_INCLUDE_VULKAN

// add libraries
#include <GLFW/glfw3.h>
#include <string_view>

namespace elementalEngine {
class WindowHandling {
public:
  WindowHandling(int w, int h, std::string_view name); // width, height, name
  ~WindowHandling();

  // cleaning up
  WindowHandling(const WindowHandling &) = delete;
  WindowHandling &operator=(const WindowHandling &) = delete;

  // getters and public functions
  bool shouldClose() { return glfwWindowShouldClose(window); }
  GLFWwindow *getGLFWwindow() { return window; }

  bool isResized() const { return framebufferResized; }
  void resetResizedFlag() { framebufferResized = false; }
  bool isMinimized() const;

private:
  // declare variables
  GLFWwindow *window;
  const int width;
  const int height;
  bool framebufferResized{false};

  // declare functions
  void initWindow(std::string_view name);
  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
};
} // namespace elementalEngine