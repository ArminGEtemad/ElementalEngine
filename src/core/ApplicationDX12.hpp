#pragma once

// header files
#include "Window.hpp"

namespace elementalEngine {
class ApplicationDX12 {
public:
  // TODO for now the window is not resizable
  // change it later after triangle is up
  static constexpr int WIDTH{1000};
  static constexpr int HEIGHT{800};

  ApplicationDX12();
  ~ApplicationDX12();

  // cleaning up
  ApplicationDX12(const ApplicationDX12 &) = delete;
  ApplicationDX12 &operator=(const ApplicationDX12 &) = delete;

  // functions
  void run();

private:
  // --- initialization ---
  WindowHandling window{WIDTH, HEIGHT, "Elemental Engine - DX12"};
  void initDX12();
};
} // namespace elementalEngine