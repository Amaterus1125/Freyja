# Freyja

A dual-backend graphics engine, developed in parallel across two languages and two APIs as a way to learn both stacks side by side.

> **Status:** 🚧 Actively in development. APIs, structure, and features are all subject to change.

## Overview

Freyja is being built as two independent implementations of the same engine:

| Implementation | Language | Graphics API | Role |
|---|---|---|---|
| `freyja-vk` | [Odin](https://odin-lang.org/) | Vulkan | Primary / low-level backend |
| `freyja-gl` | C++ | OpenGL | Reference / contribution backend |

The idea: the Vulkan side in Odin is where the "real" engine work happens — explicit control over the GPU, no hidden driver magic. The OpenGL side in C++ exists as a simpler, faster-to-prototype counterpart for testing ideas, onboarding contributors, and sanity-checking rendering results against a much less error-prone API before porting the approach over to Vulkan.

## Repository Layout

```
freyja/
├── freyja-vk/        # Odin + Vulkan implementation
│   ├── main.odin
│   └── ...
├── freyja-gl/         # C++ + OpenGL implementation
│   ├── main.cpp
│   ├── CMakeLists.txt
│   └── ...
└── README.md
```

*(Layout will grow as the project does — this reflects the current early structure.)*

## Building

### freyja-vk (Odin / Vulkan)
Requires the [Odin compiler](https://odin-lang.org/docs/install/) and the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home).

```
cd freyja-vk
odin run .
```

### freyja-gl (C++ / OpenGL)
Requires CMake and a C++17 compiler. GLFW is fetched automatically; GLAD must be generated once (see `freyja-gl/README.md` for the one-time setup step).

```
cd freyja-gl
cmake -B build
cmake --build build
```

## Roadmap

- [ ] Core windowing + swapchain/context setup (both backends)
- [ ] Basic triangle rendering (both backends)
- [ ] Shared math/scene layer
- [ ] Model loading
- [ ] Lighting
- [ ] Feature parity checkpoint between backends

## Why two backends?

Vulkan is explicit and verbose by design — great for learning what a GPU actually does, but slow to iterate with. OpenGL trades that control for speed of iteration. Building both side by side keeps the Vulkan backend honest (compare output against a known-simple reference) while keeping iteration fast enough to actually experiment.

## License

TBD.
