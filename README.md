# CUDA-Accelerated Image Processor

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![CUDA](https://img.shields.io/badge/CUDA-Upcoming-green.svg)
![Status](https://img.shields.io/badge/Status-CPU_Baseline_Complete-orange.svg)

## Overview

This repository contains a high-performance image processing application designed from scratch to apply mathematical convolution filters (such as Gaussian Blur) to massive, high-resolution images.

Rather than relying on high-level computer vision libraries, this project reads raw `P6` binary PPM files directly into dynamically allocated, flattened 1D heap memory. This architecture was explicitly chosen to mirror the strict 1D memory layout of GPU Global Memory, laying the foundation for a highly optimized CUDA port.

## Current Architecture: CPU Baseline

The current implementation serves as the foundational C++ baseline. It successfully:

* Parses strict ASCII headers to determine image metadata.
* Uses pointer casting (`reinterpret_cast<char*>`) to bypass byte-by-byte reading, achieving massive bulk-memory ingestion of 124MB+ binaries directly into RAM.
* Flattens 2D coordinates `(Row * Width + Column)` to correctly map a 3x3 convolution kernel across a 1D vector of RGB byte data.

### CPU Benchmark Data

To accurately measure the performance gains of parallelization, baseline execution times were recorded on the native CPU using a 12.1 Megapixel (3024 x 4032) input image.

The data confirms a strict **O(N) linear time complexity**, acting as the primary bottleneck that the upcoming CUDA implementation will solve.

| Passes | Execution Time (ms) | 
| ----- | ----- |
| 1 | 888.38 | 
| 5 | 4,377.26 | 
| 10 | 8,783.80 | 
| 20 | 17,610.90 | 
| 50 | 43,980.60 | 

## Roadmap: GPU Acceleration (CUDA)

The next phase of this project involves porting the CPU `applyGaussianBlur` function to the GPU to break the linear scaling barrier.

**Upcoming CUDA Optimizations:**

* [ ] **Thread Hierarchy:** Mapping the 1D image vector to a 2D grid of thread blocks.
* [ ] **Global Memory Coalescing:** Ensuring sequential threads access sequential memory addresses to maximize memory bandwidth.
* [ ] **L1 Shared Memory Tiling:** Loading image chunks into ultra-fast on-chip shared memory to minimize redundant global memory reads for overlapping kernel boundaries.
* [ ] **Halo Cells:** Handling boundary conditions efficiently within thread blocks to prevent segmentation faults.

## Build & Run Instructions

**1. Clone the repository:**
```bash
git clone https://github.com/js767011/CUDA---Accelerated-Image-Processor.git
cd CUDA---Accelerated-Image-Processor
```

**2. Compile the CPU baseline:**
```bash
g++ -std=c++17 cpu_blur.cpp -o cpu_blur
```

**3. Run the executable:**
*(Note: Requires a raw `P6` binary PPM file named `input.ppm` in the root directory)*
```bash
./cpu_blur <number_of_passes>
```
*Example: `./cpu_blur 10` will run 10 passes of the Gaussian blur and output `output_blurred.ppm`.*