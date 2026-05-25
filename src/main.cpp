#include "core/ApplicationDX12.hpp"
#include "core/ApplicationVK.hpp"

#include <cstdlib>
#include <iostream>

#define USE_DX12 true

int main() {
#if USE_DX12
  elementalEngine::ApplicationDX12 app{};
#else
  elementalEngine::ApplicationVK app{};
#endif

  try {
    app.run();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}