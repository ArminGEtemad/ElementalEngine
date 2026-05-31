#pragma once

#include <string>
#include <vector>

namespace elementalEngine::Core {

// helper function to read shader
std::vector<char> readFile(const std::string &filepath);

} // namespace elementalEngine::Core