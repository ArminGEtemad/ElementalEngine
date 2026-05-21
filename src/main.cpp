#include "core/Application.hpp"

#include <cstdlib>
#include <iostream>

int main() {
  elementalEngine::Application app{};

  try {
    app.run();

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}