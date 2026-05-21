#pragma once

// header files
#include "Window.hpp"

namespace elementalEngine {
class Application {
public:
  // TODO for now the window is not resizable
  // change it later after triangle is up
  static constexpr int WIDTH{1000};
  static constexpr int HEIGHT{800};

  Application();
  ~Application();

  // cleaning up
  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  // functions
  void run();

private:
  // initialization
  WindowHandling window{WIDTH, HEIGHT, "Elemental Engine"};
};
} // namespace elementalEngine