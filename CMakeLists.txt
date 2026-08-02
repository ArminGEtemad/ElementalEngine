cmake_minimum_required(VERSION 3.10)
project(Elemental_Engine)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# dependencies and directories
find_package(glfw3 REQUIRED)
include_directories(src src/core src/rhi src/physics src/renderer)

# everything independent of the backend
set(ENGINE_SOURCES
    src/main.cpp
    src/core/Window.cpp
    src/core/FileHandling.cpp
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
    message(FATAL_ERROR "DXC not found! Is Windows SDK or DXC in PATH?")
endif()

set(ALL_SHADER_OUTPUTS "")
set(SHADER_HEADERS 
    "${CMAKE_SOURCE_DIR}/assets/shaders/clavet_common.hlsli"
)

set(GRAPHICS_SHADERS)

foreach(SHADER_NAME IN LISTS GRAPHICS_SHADERS)

    # Vertex Shader (all will be ending in _vs)
    set(VS_SRC "${CMAKE_SOURCE_DIR}/assets/shaders/${SHADER_NAME}_vs.hlsl")
    
    if(ENABLE_VULKAN)
        set(SPV_VS_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}_vs.spv")
        add_custom_command(
            OUTPUT ${SPV_VS_OUT}
            COMMAND ${DXC_EXECUTABLE} -D__SPIRV__ -spirv -fspv-target-env=vulkan1.3 -T vs_6_0 -E VSMain ${VS_SRC} -Fo ${SPV_VS_OUT}
            DEPENDS ${VS_SRC}
            COMMENT "Compiling Vertex Shader (${SHADER_NAME}_vs) to SPIR-V..."
        )
        list(APPEND ALL_SHADER_OUTPUTS ${SPV_VS_OUT})
    endif()

    if(ENABLE_DX12)
        set(DXIL_VS_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}_vs.dxil")
        add_custom_command(
            OUTPUT ${DXIL_VS_OUT}
            COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain ${VS_SRC} -Fo ${DXIL_VS_OUT}
            DEPENDS ${VS_SRC}
            COMMENT "Compiling Vertex Shader (${SHADER_NAME}_vs) to DXIL..."
        )
        list(APPEND ALL_SHADER_OUTPUTS ${DXIL_VS_OUT})
    endif()

    # Fragment Shader (all will be ending in _fs)
    set(FS_SRC "${CMAKE_SOURCE_DIR}/assets/shaders/${SHADER_NAME}_fs.hlsl")
    
    if(ENABLE_VULKAN)
        set(SPV_FS_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}_fs.spv")
        add_custom_command(
            OUTPUT ${SPV_FS_OUT}
            COMMAND ${DXC_EXECUTABLE} -D__SPIRV__ -spirv -fspv-target-env=vulkan1.3 -T ps_6_0 -E FSMain ${FS_SRC} -Fo ${SPV_FS_OUT}
            DEPENDS ${FS_SRC}
            COMMENT "Compiling Fragment Shader (${SHADER_NAME}_fs) to SPIR-V..."
        )
        list(APPEND ALL_SHADER_OUTPUTS ${SPV_FS_OUT})
    endif()

    if(ENABLE_DX12)
        set(DXIL_FS_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}_fs.dxil")
        add_custom_command(
            OUTPUT ${DXIL_FS_OUT}
            COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E FSMain ${FS_SRC} -Fo ${DXIL_FS_OUT}
            DEPENDS ${FS_SRC}
            COMMENT "Compiling Fragment Shader (${SHADER_NAME}_fs) to DXIL..."
        )
        list(APPEND ALL_SHADER_OUTPUTS ${DXIL_FS_OUT})
    endif()
endforeach()

# compute Shaders
set(COMPUTE_SHADERS)

foreach(SHADER_NAME IN LISTS COMPUTE_SHADERS)
    set(HLSL_SRC "${CMAKE_SOURCE_DIR}/assets/shaders/${SHADER_NAME}.hlsl")
    
    if(ENABLE_VULKAN)
        set(SPV_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${DXC_EXECUTABLE} -D__SPIRV__ -spirv -fspv-target-env=vulkan1.3 -T cs_6_0 -E CSMain ${HLSL_SRC} -Fo ${SPV_OUT}
            DEPENDS ${HLSL_SRC}
            COMMENT "Compiling Compute Shader (${SHADER_NAME}) to SPIR-V..."
        )
        list(APPEND ALL_SHADER_OUTPUTS ${SPV_OUT})
    endif()

    if(ENABLE_DX12)
        set(DXIL_OUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}.dxil")
        add_custom_command(
            OUTPUT ${DXIL_OUT}
            COMMAND ${DXC_EXECUTABLE} -T cs_6_0 -E CSMain ${HLSL_SRC} -Fo ${DXIL_OUT}
            DEPENDS ${HLSL_SRC}
            COMMENT "Compiling Compute Shader (${SHADER_NAME}) to DXIL..."
        )
        list(APPEND ALL_SHADER_OUTPUTS ${DXIL_OUT})
    endif()
endforeach()

if(ALL_SHADER_OUTPUTS)
    add_custom_target(CompileShaders ALL DEPENDS ${ALL_SHADER_OUTPUTS})
    add_dependencies(ElementalEngine CompileShaders)
endif()