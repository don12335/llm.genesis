# LLM.Genesis: Native Inference Substrate

> ### (https://github.com/don12335/Genesis)

LLM.Genesis is a high-performance, self-contained inference engine designed to execute large language models within a minimalist virtual environment. By utilizing a custom Genesis Compute System (GCS) Virtual Machine, the project achieves hardware-agnostic, zero-dependency inference on a strict 64KB SRAM memory substrate.

## Overview

Unlike traditional inference frameworks that rely on heavy libraries and complex runtime environments, LLM.Genesis operates at the instruction level. It treats model logic as executable "DNA," allowing for direct manipulation of the inference process, dynamic weight loading, and stateful sequence generation within a highly constrained memory footprint.

## Core Components

### 1. Native Inference Engine (C++)
The core of the system is a specialized GCS Virtual Machine implemented in standard C++. 
- **Memory Management**: Optimized for 64KB SRAM architecture.
- **Instruction Set**: Includes dedicated opcodes for matrix multiplication (MATMUL), float operations (FADD, FMUL), and non-linear activations (SILU, RMSNORM).
- **Hard-wired Efficiency**: Direct memory-to-tensor mapping bypassing traditional overhead.

### 2. Genetic Coding System (GCS)
Model topology and inference logic are compiled into a binary format known as GCS DNA.
- **Dynamic Streaming**: Supports the STREAM opcode for paged loading of multi-megabyte weight files into limited SRAM windows.
- **Logic Autonomy**: The GCS DNA defines the exact sequence of the transformer block, allowing the model to be updated without altering the runner's source code.

### 3. Training & Compilation Pipeline (Python)
- **Sovereign Trainer**: A minimalist training script to prepare weights for the GCS substrate.
- **Manifest Compiler**: Translates sequence logic and weight offsets into the GCS binary format.

## Technical Specifications

- **Target Architecture**: 64KB SRAM Segmented Memory.
- **Precision**: 32-bit Floating Point (FPU Emulated/Native).
- **Latency**: Sub-millisecond opcode execution on standard hardware.
- **Dependencies**: None (Requires only a C++17 compliant compiler).

## Installation and Build

### Prerequisites
- G++ or any C++17 compliant compiler.
- Python 3.8+ (for tooling and compilation).

### Building the Runner
Execute the following command to compile the native inference engine:
```powershell
g++ -std=c++17 -O2 src/main.cpp src/vm.cpp -o bin/llm_genesis.exe
```

### Compiling logic DNA
```powershell
python tools/compiler_manifest.py
```

## Design Philosophy

LLM.Genesis is built on the principle of computational sovereignty. By reducing the inference engine to its most fundamental primitives, the system ensures that intelligence can reside locally, operate predictably, and remain entirely independent of external cloud infrastructures or proprietary stack dependencies.
