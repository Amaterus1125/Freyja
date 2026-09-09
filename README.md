<div align="center">

# ✦ Freyja ✦

**A dual-backend graphics engine with one core, two GPU philosophies.**

[![Odin](https://img.shields.io/badge/Odin-Vulkan-3ba3ec?style=for-the-badge)](https://odin-lang.org/)
[![C++](https://img.shields.io/badge/C%2B%2B-OpenGL-00599C?style=for-the-badge&logo=cplusplus)](https://isocpp.org/)
[![Status](https://img.shields.io/badge/status-in%20development-orange?style=for-the-badge)]()
[![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)]()

<br>

<img src="https://imgs.search.brave.com/CSi1h98kPP4oCKilbZRp4hQGq9qcRswu43Uvei6414E/rs:fit:860:0:0:0/g:ce/aHR0cHM6Ly9jZG4u/Z2V0cGZwcy5jb20v/aW1nL21vYmlsZS9t/aW5pbWFsaXN0LXBm/cHMvZ2VvbWV0cmlj/LXNoYXBlcy9jcnlz/dGFsLWNsZWFyLXdh/dGVyLXNwbGFzaC1n/ZW9tZXRyaWMtcGl4/ZWwtYXJ0LW1vYmls/ZS0xenZobzItNTU1/NTU4OTQud2VicA" alt="Freyja banner" width="400">



</div>

---

## ✦ What is Freyja?

Freyja is being built as **two independent implementations of the same engine**, developed in parallel so each one keeps the other honest:

| | Branch | Language | API | Role |
|---|---|---|---|---|
| 🗡️ | `main` | [Odin](https://odin-lang.org/) | **Vulkan** | Primary engine — explicit, low-level, no hidden driver magic |
| 🛡️ | `opengl` *(own master)* | C++ | **OpenGL** | Reference / contribution backend — fast to prototype, easy to onboard into |

> The Vulkan side is where the real engine work happens. The OpenGL branch exists as a simpler counterpart for testing ideas and sanity-checking rendering results before the same approach gets ported over to Vulkan — and as a friendlier entry point for anyone contributing who doesn't want to wade through Vulkan boilerplate on day one.

---

## ✦ Branches

Freyja is split across branches rather than folders — each backend is developed as its own line of history:

```
main     → Odin + Vulkan   (primary engine)
Cpp   → C++ + OpenGL    (independent master branch, its own history/releases)
```

Clone and check out whichever backend you're working on:

```bash
git clone https://github.com/Amaterus1125/freyja.git

# Odin / Vulkan (default)
cd freyja

# C++ / OpenGL
git checkout opengl
```

---

## ✦ Building

<table>
<tr>
<td width="50%" valign="top">

### 🗡️ `main` — Odin / Vulkan

Requires:
- [Odin compiler](https://odin-lang.org/docs/install/)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

```bash
odin run .
```

</td>
<td width="50%" valign="top">

### 🛡️ `Cpp` — C++ / OpenGL

Requires:
- CMake + a C++17 compiler
- GLAD generated once (see branch README)

```bash
cmake -B build
cmake --build build
```

</td>
</tr>
</table>

---

## ✦ Roadmap

- [ ] Core windowing + swapchain/context setup (both backends)
- [ ] Basic triangle rendering (both backends)
- [ ] Shared math / scene layer
- [ ] Model loading
- [ ] Lighting
- [ ] Feature-parity checkpoint between backends

---

## ✦ Why two backends?

Vulkan is explicit and verbose by design — excellent for learning exactly what the GPU is doing, but slow to iterate with. OpenGL trades that control for speed. Building both side by side keeps the Vulkan engine honest against a known-simple reference, while keeping iteration fast enough to actually experiment with ideas before committing them to the "real" backend.

---

<div align="center">

**License:** [MIT](LICENSE) &nbsp;•&nbsp; **Status:** actively in development, expect breaking changes

</div>
