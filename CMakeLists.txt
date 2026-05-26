cmake_minimum_required(VERSION 3.10)
project(Elemental_Engine)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(src src/core src/rhi)
set(DX12_LIBS d3d12 dxgi)

add_executable(ElementalEngine
    src/main.cpp
    src/rhi/Device.cpp
    src/core/ApplicationVK.cpp
    src/core/ApplicationDX12.cpp
    src/rhi/vulkan/VulkanDevice.cpp
    src/rhi/dx12/DX12Device.cpp
    src/core/Window.cpp    
)

find_program(DXC_EXECUTABLE 
    NAMES dxc
    PATHS "$ENV{VULKAN_SDK}/Bin"
    NO_DEFAULT_PATH
)

if(NOT DXC_EXECUTABLE)
    find_program(DXC_EXECUTABLE NAMES dxc)
endif()

if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "DXC not found! Is Windows SDK or DXC in PATH?")
endif()


set(SHADER_SRC "${CMAKE_SOURCE_DIR}/assets/shaders/triangle.hlsl")
set(SPV_VERT_OUT "${CMAKE_BINARY_DIR}/triangle_vs.spv")
set(SPV_FRAG_OUT "${CMAKE_BINARY_DIR}/triangle_ps.spv")
set(DXIL_VERT_OUT "${CMAKE_BINARY_DIR}/triangle_vs.dxil")
set(DXIL_FRAG_OUT "${CMAKE_BINARY_DIR}/triangle_ps.dxil")

add_custom_command(
    OUTPUT ${SPV_VERT_OUT}
    COMMAND ${DXC_EXECUTABLE} -spirv -T vs_6_0 -E VSMain ${SHADER_SRC} -Fo ${SPV_VERT_OUT}
    DEPENDS ${SHADER_SRC}
    COMMENT "Compiling HLSL Vertex Shader to SPIR-V..."
)

add_custom_command(
    OUTPUT ${SPV_FRAG_OUT}
    COMMAND ${DXC_EXECUTABLE} -spirv -T ps_6_0 -E PSMain ${SHADER_SRC} -Fo ${SPV_FRAG_OUT}
    DEPENDS ${SHADER_SRC}
    COMMENT "Compiling HLSL Fragment Shader to SPIR-V..."
)

add_custom_command(
    OUTPUT ${DXIL_VERT_OUT}
    COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain ${SHADER_SRC} -Fo ${DXIL_VERT_OUT}
    DEPENDS ${SHADER_SRC}
    COMMENT "Compiling HLSL Vertex Shader to DXIL..."
)

add_custom_command(
    OUTPUT ${DXIL_FRAG_OUT}
    COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E PSMain ${SHADER_SRC} -Fo ${DXIL_FRAG_OUT}
    DEPENDS ${SHADER_SRC}
    COMMENT "Compiling HLSL Fragment Shader to DXIL..."
)
add_custom_target(CompileShaders ALL DEPENDS ${SPV_VERT_OUT} ${SPV_FRAG_OUT} ${DXIL_VERT_OUT} ${DXIL_FRAG_OUT})

add_dependencies(ElementalEngine CompileShaders)
target_link_libraries(ElementalEngine Vulkan::Vulkan glfw d3d12 dxgi dxguid)