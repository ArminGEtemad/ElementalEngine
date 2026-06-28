# Elemental Engine

> This project is my most ambitious project until now.
>
> Target: This engine must be finished around the time I finish my PhD.

A real-time, high-performance **Systemic Multi-Element Reactivity Engine** built from scratch in modern C++ to simulate complex parallel thermodynamic and chemical phase transitions directly on parallel GPU.

I will be writing the engine without any use of game engines. I will use **Hardware Abstraction Layer (HAL)** to not rewrite the whole code in both backends.

## Poison Gas (finished as isolated element)

<div style="display: flex; gap: 100px; align-items: flex-start;">

  <div>
    <img src="PicturesAndGifs/vulkan smoke.png" width="800"/>
  </div>

</div>

## Core Architecture & Pipeline Layout

I will be using two backends

- **Vulkan 1.3 Backend**
- **DirectX 12 Backend**

### Used Hardware

**Development Device:** Intel Core CPU / NVIDIA RTX 4070 Ti Super (Pop!\_OS Linux & Windows 11)

**Additionally:** I might add an ASUS handheld with AMD to make sure that Nvidia is not being merciful.

## Simulation Framework (The GPU Kernels)

The element mechanics (Fire, Water, Electricity, Gas) are evaluated inside a 3D grid cell topology utilizing double-buffered GPU Structured Buffers. (Compute and Fragment Shader)

My idea is that, the fire can make the gas explode, the water can turn off the fire, the electricity can electrify the water, the fire can evaporate the water etc.

## Moving Forward

The following is how I would like to move forward with the project

- [x] Set up windowing (GLFW/SDL), swapchains, and device initialization for both Vulkan 1.3 and DX12.
- [x] Build the thin HAL. Get a basic triangle rendering in both APIs to verify the pipeline.
- [x] Implement Compute Shader dispatching in the HAL. Set up structured buffers and read/write textures.
- [x] Write a basic advection and diffusion compute shader to move generic "density" around.
- [x] Fluid Dynamics (Poison Gas)

I am changing some of the things I wanted to make.
Instead of a generic gas I would like to add a liquid(acid) projectil that upon impact functions as a source for the poison gas.

for this matter I think I need the followings:

- [ ] Collision Geometry (Dirichlet boundary condition)
- [ ] blob of acid buffer
- [ ] right now I am using Joe Stam's algorithm but the projectile is not dissipative (I found another paper! if it works I will cite them!)
- [ ] phase change compute pass
