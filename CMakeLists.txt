cmake_minimum_required(VERSION 3.10)
project(Elemental_Engine)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(D3D12MemoryAllocator CONFIG REQUIRED)

include_directories(src src/core src/rhi)
set(DX12_LIBS d3d12 dxgi)

add_executable(ElementalEngine
    src/main.cpp
    src/core/Window.cpp
    src/core/FileHandling.cpp
    src/rhi/Device.cpp
    src/rhi/vulkan/VulkanDevice.cpp
    src/rhi/vulkan/VulkanSwapchain.cpp
    src/rhi/vulkan/VulkanCommandList.cpp
    src/rhi/vulkan/VulkanPipeline.cpp
    src/rhi/vulkan/VulkanComputePipeline.cpp
    src/rhi/vulkan/VulkanBuffer.cpp
    src/rhi/vulkan/VulkanAllocator.cpp
    src/rhi/dx12/DX12Device.cpp
    src/rhi/dx12/DX12Swapchain.cpp
    src/rhi/dx12/DX12CommandList.cpp
    src/rhi/dx12/DX12Pipeline.cpp
    src/rhi/dx12/DX12ComputePipeline.cpp
    src/rhi/dx12/DX12Buffer.cpp
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


set(SHADER_GRID_VERT "${CMAKE_SOURCE_DIR}/assets/shaders/grid_vs.hlsl")
set(SHADER_GRID_FRAG "${CMAKE_SOURCE_DIR}/assets/shaders/grid_fs.hlsl")
set(SHADER_GRID_COMP "${CMAKE_SOURCE_DIR}/assets/shaders/advection.hlsl")

set(SPV_VERT_OUT "${CMAKE_BINARY_DIR}/grid_vs.spv")
set(SPV_FRAG_OUT "${CMAKE_BINARY_DIR}/grid_fs.spv")
set(SPV_COMP_OUT "${CMAKE_BINARY_DIR}/advection.spv")

set(DXIL_VERT_OUT "${CMAKE_BINARY_DIR}/grid_vs.dxil")
set(DXIL_FRAG_OUT "${CMAKE_BINARY_DIR}/grid_fs.dxil")
set(DXIL_COMP_OUT "${CMAKE_BINARY_DIR}/advection.dxil")

add_custom_command(
    OUTPUT ${SPV_VERT_OUT}
    COMMAND ${DXC_EXECUTABLE} -spirv -fspv-target-env=vulkan1.3 -T vs_6_0 -E VSMain ${SHADER_GRID_VERT} -Fo ${SPV_VERT_OUT}
    DEPENDS ${SHADER_GRID_VERT}
    COMMENT "Compiling HLSL Vertex Shader to SPIR-V..."
)
add_custom_command(
    OUTPUT ${SPV_FRAG_OUT}
    COMMAND ${DXC_EXECUTABLE} -D__SPIRV__ -spirv -fspv-target-env=vulkan1.3 -T ps_6_0 -E FSMain ${SHADER_GRID_FRAG} -Fo ${SPV_FRAG_OUT}
    DEPENDS ${SHADER_GRID_FRAG}
    COMMENT "Compiling HLSL Fragment Shader to SPIR-V..."
)
add_custom_command(
    OUTPUT ${SPV_COMP_OUT}
    COMMAND ${DXC_EXECUTABLE} -D__SPIRV__ -spirv -fspv-target-env=vulkan1.3 -T cs_6_0 -E CSMain ${SHADER_GRID_COMP} -Fo ${SPV_COMP_OUT}
    DEPENDS ${SHADER_GRID_COMP}
    COMMENT "Compiling HLSL Compute Shader to SPIR-V..."
)

add_custom_command(
    OUTPUT ${DXIL_VERT_OUT}
    COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain ${SHADER_GRID_VERT} -Fo ${DXIL_VERT_OUT}
    DEPENDS ${SHADER_GRID_VERT}
    COMMENT "Compiling HLSL Vertex Shader to DXIL..."
)
add_custom_command(
    OUTPUT ${DXIL_FRAG_OUT}
    COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E FSMain ${SHADER_GRID_FRAG} -Fo ${DXIL_FRAG_OUT}
    DEPENDS ${SHADER_GRID_FRAG}
    COMMENT "Compiling HLSL Fragment Shader to DXIL..."
)
add_custom_command(
    OUTPUT ${DXIL_COMP_OUT}
    COMMAND ${DXC_EXECUTABLE} -T cs_6_0 -E CSMain ${SHADER_GRID_COMP} -Fo ${DXIL_COMP_OUT}
    DEPENDS ${SHADER_GRID_COMP}
    COMMENT "Compiling HLSL Compute Shader to DXIL..."
)

add_custom_target(CompileShaders ALL DEPENDS ${SPV_VERT_OUT} ${SPV_FRAG_OUT} ${SPV_COMP_OUT} ${DXIL_VERT_OUT} ${DXIL_FRAG_OUT} ${DXIL_COMP_OUT})

add_dependencies(ElementalEngine CompileShaders)
target_link_libraries(ElementalEngine
    Vulkan::Vulkan 
    glfw 
    d3d12 
    dxgi 
    dxguid 
    GPUOpen::VulkanMemoryAllocator 
    GPUOpen::D3D12MemoryAllocator
)