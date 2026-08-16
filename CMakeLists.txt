cmake_minimum_required(VERSION 3.10)
project(Elemental_Engine)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# dependencies and directories
find_package(glfw3 REQUIRED)
include_directories(src src/core src/rhi src/physics src/graphics)

# everything independent of the backend
set(ENGINE_SOURCES
    src/main.cpp
    src/core/Window.cpp
    src/core/FileHandling.cpp
    src/core/Camera.cpp
    src/rhi/Device.cpp
)

set(ENGINE_LIBS glfw)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

list(APPEND ENGINE_SOURCES
    src/rhi/vulkan/VulkanDevice.cpp
    src/rhi/vulkan/VulkanSwapchain.cpp
    src/rhi/vulkan/VulkanCommandList.cpp
    src/rhi/vulkan/VulkanPipeline.cpp
    src/rhi/vulkan/VulkanComputePipeline.cpp
    src/rhi/vulkan/VulkanBuffer.cpp
    src/rhi/vulkan/VulkanAllocator.cpp
    src/rhi/vulkan/VulkanTexture.cpp
)

# whatever has to do with graphics and compute passes
list(APPEND ENGINE_SOURCES
    src/graphics/TerrainPass.cpp
    src/graphics/PBFSlimeRenderer.cpp
    src/physics/PBFSlime.cpp
)

list(APPEND ENGINE_LIBS Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)

# executables
add_executable(ElementalEngine ${ENGINE_SOURCES})
target_link_libraries(ElementalEngine PRIVATE ${ENGINE_LIBS})

# shader compiling directories
find_program(DXC_EXECUTABLE 
    NAMES dxc
    PATHS "$ENV{VULKAN_SDK}/Bin"
    NO_DEFAULT_PATH
)

if(NOT DXC_EXECUTABLE)
    find_program(DXC_EXECUTABLE NAMES dxc)
endif()

if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "DXC not found! Is Vulkan SDK or DXC in PATH?")
endif()

set(ALL_SHADER_OUTPUTS "")
set(SHADER_HEADERS 
    "${CMAKE_SOURCE_DIR}/shaders/clavet_common.hlsli"
)

# --- Graphics Shaders ---
set(GRAPHICS_SHADERS Terrain Slime_3d)

foreach(SHADER_NAME ${GRAPHICS_SHADERS})
    # Vertex Shader all must end in _vs
    set(VS_SRC "${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}_vs.hlsl")
    set(SPV_VS_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}_vs.spv")

    add_custom_command(
        OUTPUT ${SPV_VS_OUT}
        COMMAND ${DXC_EXECUTABLE} -D__SPIRV__ -spirv -fspv-target-env=vulkan1.3 -T vs_6_0 -E VSMain ${VS_SRC} -Fo ${SPV_VS_OUT}
        DEPENDS ${VS_SRC}
        COMMENT "Compiling Vertex Shader (${SHADER_NAME}_vs.hlsl) to SPIR-V..."
    )
    list(APPEND ALL_SHADER_OUTPUTS ${SPV_VS_OUT})

    set(FS_SRC "${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}_fs.hlsl")
    set(SPV_FS_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}_fs.spv")

    add_custom_command(
        OUTPUT ${SPV_FS_OUT}
        COMMAND ${DXC_EXECUTABLE} -D__SPIRV__ -spirv -fspv-target-env=vulkan1.3 -T ps_6_0 -E FSMain ${FS_SRC} -Fo ${SPV_FS_OUT}
        DEPENDS ${FS_SRC}
        COMMENT "Compiling Fragment Shader (${SHADER_NAME}_fs.hlsl) to SPIR-V..."
    )
    list(APPEND ALL_SHADER_OUTPUTS ${SPV_FS_OUT})
endforeach()


# --- Compute Shaders ---
set(COMPUTE_SHADERS clavet_grid_construct clavet_grid_next_construct clavet_predicted clavet_springs
    clavet_density clavet_relax clavet_integrate)

foreach(SHADER_NAME ${COMPUTE_SHADERS})
    set(HLSL_SRC "${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME}.hlsl")
    set(SPV_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPV_OUT}
        COMMAND ${DXC_EXECUTABLE} -D__SPIRV__ -spirv -fspv-target-env=vulkan1.3 -T cs_6_0 -E CSMain ${HLSL_SRC} -Fo ${SPV_OUT}
        DEPENDS ${HLSL_SRC}
        COMMENT "Compiling Compute Shader (${SHADER_NAME}.hlsl) to SPIR-V..."
    )
    list(APPEND ALL_SHADER_OUTPUTS ${SPV_OUT})
endforeach()


if(ALL_SHADER_OUTPUTS)
    add_custom_target(CompileShaders ALL DEPENDS ${ALL_SHADER_OUTPUTS})
    add_dependencies(ElementalEngine CompileShaders)
endif()