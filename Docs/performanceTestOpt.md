# Performance Documentation (Nsight Graphics)

For the profiling I moved to IMMIDIATE present instead of FIFO so Vsync does not affect the results. In every profiling the number of slime particles were 10000, and the number of fire particles were 50000. The particles initial movement was a drop with with zero intial velocity. Every profiling is on RTX GeForce 4070 Ti Super.

## ElEn version 0.99

Nsight profiling showed the

| Pipeline Phase               | Type               | Time ($\text{ms}$) | % of Frame |
| :--------------------------- | :----------------- | :----------------: | :--------: |
| **PBF Slime Simulation**     | Compute            |    **6.73 ms**     |   83.1%    |
| **Stam Fluid Simulation**    | Compute            |    **1.08 ms**     |   13.3%    |
| **Stam Fluid Raymarcher**    | Graphics           |    **0.12 ms**     |    1.5%    |
| **SSFR**                     | Graphics / Compute |    **0.06 ms**     |    0.7%    |
| **Fire Particle Renders**    | Graphics           |    **0.03 ms**     |    0.4%    |
| **Fire Particle Simulation** | Compute            |    **0.02 ms**     |    0.2%    |
| **Terrain Geometry Pass**    | Graphics           |    **<0.01 ms**    |   <0.1%    |
| **Active Frame Time**        | **Mixed**          |    **~8.09 ms**    | **~99.8%** |
| **Total Frame Time**         | **Mixed**          |    **8.10 ms**     | **100.0%** |

The biggest bottleneck is the Clavet Slime. The output stall location in LSU, Global memory showed 67% stall on loading and the texture about 20% stall on loading.

Out of 6.73 ms that the compute shader for Clavet Slime needed, 4.89 ms was used by Spring shader. with 99% stall in LSU, global memory loading the data. The math and XU pipe, special function was blazing fast with only 0.04% stall even thought it made up 16% of the instruction.

## ElEn Version 1.0.0

For the first publish, the only optimization I ended up doing was optimizing the placement of the math function `sqrt`. Of course, I did not expected a massive speed up since the math was already fast. It reduced the XU stall down to 0.02% however, the frame time and Clavet Slime eneded up being slower.

## ElEn Version 1.1.0

To actually find the bottleneck, I ran two experiments.

### Experiments A

I decided to vary the number springs. I thought this was the fastest, even thought dirty, idea to optimize the math. I tested 64, 32 and 16 number of springs. At the same time I was thinking what if the less spring the worse the performance and it make the GPU to stall for longer.

The results were as following

| MAX_SPRINGS | Spring shader | Slime simulation |
| ----------: | ------------: | ---------------: |
|          64 |      ~4.89 ms |         ~6.73 ms |
|          32 |   **1.99 ms** |      **3.41 ms** |
|          16 |   **1.34 ms** |      **2.66 ms** |

Which means that the time of calculation does not show any signs of unpredictability. Then I moved to the next experiment (even though the Experiment A already made it clear in some ways that the second half of the spring script is the bottleneck)

### Experiment B

I decided to completely remove the second part `Add new springs` of the script and do the profiling again.

Result:

Frame: 2.58 ms
Slime simulation: 1.22 ms
Spring shader: 0.02 ms
Global-memory stall: 45%

The stall is still not optimized however the frame time and the calculations are much faster. I assume the stall is due to pointer chasing.

### Optimization 1 Results

The spring script was then optimized so that we don't go through all the spring but only the active ones. Early terminations were added. In addition, the shader was changed to access only the predictedPosition member of each particle instead of copying the complete Particle structure, reducing unnecessary data movement. These changes reduce redundant global-memory accesses and avoid scanning inactive spring slots for every candidate neighbor.

the results were:

Frame: 3.70 ms
Slime simulation: 2.34 ms
Spring shader: 0.35 ms
Global-memory stall: 47%

## When fire starts

The Slime Simulation again becomes the bottleneck when the fire starts.

Frame: 5.82 ms
Slime simulation: 4.53 ms (77.8%)
Global-memory stall: 60%

| Pipeline Phase          | Time ($\text{ms}$) No fire | Time ($\text{ms}$) With fire |
| :---------------------- | :------------------------- | :--------------------------: |
| **Position Predition**  | **0.58 ms**                |         **1.30 ms**          |
| **Spring & Plasticity** | **0.40 ms**                |         **1.16 ms**          |
| **Density Phase 1**     | **0.55 ms**                |         **0.99 ms**          |
| **Density Phase 2**     | **0.57 ms**                |         **1.05 ms**          |

However this time not only the spring shader is taking longer but also density phase 1 and 2. The stall on the whole Slime simulation is over 92%. At first one would think that fire must be the bottleneck, however, the redering of the fire particles only needs 0.04ms. with a memory stall of 33% and a special function stall of 25%. Another bigger problem is that Density Phase 2 has NO FIRE logic in itself. So the fact that the calculation time is doubled is a smoking gun. The second massive problem I have seen was that the from rate drops constantly. Even when the whole slime is consumed by fire the frame rate is still down even under 100 FPS.

A healthy grid is occupied by mutiple slime particles and particles interact with each other. Visually: the more particles the more the liquid behaves like an actual slime puddle. Also when a particle is consumed by fire the math should not run for it. I realized when writing the program I totally forgot to use that fact.

## ElEn Version 1.2.0

By adding

```
if (particles[id].health <= 0.0f) {
    return;
}
```

meaning if a particle is dead, it never gets added to the linked list. I solved the problem AND I also only used the partial part of struct that was needed in the density scripts.

The new results

### Optimization 2 Results

| Pipeline Phase          | Time ($\text{ms}$) No fire | Time ($\text{ms}$) With fire |
| :---------------------- | :------------------------- | :--------------------------: |
| **Slime Simulation**    | **3.07 ms**                |         **2.14 ms**          |
| **Position Predition**  | **0.55 ms**                |         **0.37 ms**          |
| **Spring & Plasticity** | **0.90 ms**                |         **0.72 ms**          |
| **Density Phase 1**     | **0.78 ms**                |         **0.50 ms**          |
| **Density Phase 2**     | **0.81 ms**                |         **0.51 ms**          |

No Fire Global-memory stall: 58%
With Fire Global-memory stall: 48%

Meaning that the memory stall is now back up even though the frame time is lower and the script is behaving in a predictable way.
When all the slime particles are consumed we even hit 900 FPS.
