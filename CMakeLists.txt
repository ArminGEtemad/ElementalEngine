cmake_minimum_required(VERSION 3.10)
project(Elemental_Engine)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(src src/core)

add_executable(ElementalEngine
    src/main.cpp
    src/core/Application.cpp
    src/core/Window.cpp    
)

find_program(GLSLANG_VALIDATOR NAMES glslangValidator)

if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found! Is the Vulkan SDK installed?")
endif()

target_link_libraries(ElementalEngine Vulkan::Vulkan glfw)