cmake_minimum_required(VERSION 3.10)
project(Elemental_Engine)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# making option for vulkan and DX backend ON, OFF
set(ENABLE_VULKAN ON)
set(ENABLE_DX12 ON)

if(NOT ENABLE_VULKAN AND NOT ENABLE_DX12)
    message(FATAL_ERROR "At least one rendering backend (ENABLE_VULKAN or ENABLE_DX12) must be turned ON!")
endif()

# dependencies and directories
find_package(glfw3 REQUIRED)
include_directories(src src/core src/rhi src/physics)

# --- independent of backend scripts ---
set(ENGINE_SOURCES
    src/main.cpp
    src/core/Window.cpp
    src/core/FileHandling.cpp
    src/rhi/Device.cpp
    src/physics/stamFluid.cpp
    #src/physics/PBFSlime.cpp
)

set(ENGINE_LIBS glfw)

# Vulkan backend
if(ENABLE_VULKAN)
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
endif()

# DX12
if(ENABLE_DX12)
    find_package(D3D12MemoryAllocator CONFIG REQUIRED)
    
    list(APPEND ENGINE_SOURCES
        src/rhi/dx12/DX12Device.cpp
        src/rhi/dx12/DX12Swapchain.cpp
        src/rhi/dx12/DX12CommandList.cpp
        src/rhi/dx12/DX12Pipeline.cpp
        src/rhi/dx12/DX12ComputePipeline.cpp
        src/rhi/dx12/DX12Buffer.cpp
        src/rhi/dx12/DX12Texture.cpp
    )
    list(APPEND ENGINE_LIBS d3d12 dxgi dxguid GPUOpen::D3D12MemoryAllocator)
endif()

# executables
add_executable(ElementalEngine ${ENGINE_SOURCES})
target_link_libraries(ElementalEngine PRIVATE ${ENGINE_LIBS})

# macro in code
if(ENABLE_VULKAN)
    target_compile_definitions(ElementalEngine PRIVATE RHI_VULKAN_ENABLED)
endif()
if(ENABLE_DX12)
    target_compile_definitions(ElementalEngine PRIVATE RHI_DX12_ENABLED)
endif()

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

# shaders fragment and vertex
set(SHADER_GRID_VERT "${CMAKE_SOURCE_DIR}/assets/shaders/grid_vs.hlsl")
set(SHADER_GRID_FRAG "${CMAKE_SOURCE_DIR}/assets/shaders/grid_fs.hlsl")

if(ENABLE_VULKAN)
    set(SPV_VERT_OUT "${CMAKE_BINARY_DIR}/grid_vs.spv")
    set(SPV_FRAG_OUT "${CMAKE_BINARY_DIR}/grid_fs.spv")
    
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
    list(APPEND ALL_SHADER_OUTPUTS ${SPV_VERT_OUT} ${SPV_FRAG_OUT})
endif()

if(ENABLE_DX12)
    set(DXIL_VERT_OUT "${CMAKE_BINARY_DIR}/grid_vs.dxil")
    set(DXIL_FRAG_OUT "${CMAKE_BINARY_DIR}/grid_fs.dxil")
    
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
    list(APPEND ALL_SHADER_OUTPUTS ${DXIL_VERT_OUT} ${DXIL_FRAG_OUT})
endif()

# compute Shaders
set(COMPUTE_SHADERS advection divergence jacobi gradient)

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