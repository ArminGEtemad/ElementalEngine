#include "FileHandling.hpp"

#include <fstream>
#include <iostream>

namespace elementalEngine::Core {

std::vector<char> readFile(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file!");
  }
  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();
  return buffer;
}
} // namespace elementalEngine::Core