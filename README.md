# Elemental Engine

A real-time, compute-driven **Multi-Physics & Reactivity Engine** built from scratch in C++20 and **Vulkan 1.3**, featuring coupled multi-element interactions (gas, viscoelastic fluids, electricity, thermodynamics) and a custom Render Hardware Interface (RHI).

> version 1.2.0

## GIF Showcase

<div style="display: flex; gap: 100px; align-items: flex-start;">

  <div>
    <img src="PicturesAndGifs/Version1_2_0_Gif.gif" width="800"/>
  </div>

</div>

## Technical Highlights

- **Custom Low-Level RHI:** Thin hardware abstraction designed around modern explicit graphics APIs (Vulkan 1.3 native, DX12-ready architecture).
- **Modern Vulkan 1.3 Core:**
  - Full **Dynamic Rendering** pipeline (`VK_KHR_dynamic_rendering`), removing legacy render pass boilerplate.
  - Utilization of compute shaders.
  - Integrated **Vulkan Memory Allocator (VMA)** for memory management.
  - Vertex Pulling is used instead of Vertex Buffers.
- **Hierarchical GPU Profiling:** Instrumented with `VK_EXT_debug_utils` for nested event hierarchy in RenderDoc and Nsight.

## Performance & Metrics

**Benchmarked on:** NVIDIA GeForce RTX 4070 Ti Super / Intel Core CPU i7

| Pipeline Phase               | Type               | Time ($\text{ms}$) |
| :--------------------------- | :----------------- | :----------------: |
| **PBF Slime Simulation**     | Compute            |    **2.14 ms**     |
| **Stam Fluid Simulation**    | Compute            |    **1.32 ms**     |
| **Stam Fluid Raymarcher**    | Graphics           |    **0.11 ms**     |
| **SSFR**                     | Graphics / Compute |    **0.04 ms**     |
| **Fire Particle Renders**    | Graphics           |    **0.04 ms**     |
| **Fire Particle Simulation** | Compute            |    **0.02 ms**     |
| **Terrain Geometry Pass**    | Graphics           |    **<0.01 ms**    |
| **Active Frame Time**        | **Mixed**          |    **~3.72 ms**    |
| **Total Frame Time**         | **Mixed**          |    **~3.73 ms**    |

The optimizations steps are given in [Docs](Docs/performaceTestOpt.md).

## Dependencies

I used VMA (Vulkan Memory Allocator) for memory allocations. You can find it [here](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) and install it.

## Building from Source

```bash
git clone https://github.com/ArminGEtemad/ElementalEngine.git
cd ElementalEngine
mkdir build && cd build
cmake ..
cmake --build . --config Release
```
