<div align="center">

# ✦ Freyja ✦

**A dual-backend graphics engine with one core, two GPU philosophies.**

[![Odin](https://img.shields.io/badge/Odin-Vulkan-3ba3ec?style=for-the-badge)](https://odin-lang.org/)
[![C++](https://img.shields.io/badge/C%2B%2B-OpenGL-00599C?style=for-the-badge&logo=cplusplus)](https://isocpp.org/)
[![Status](https://img.shields.io/badge/status-in%20development-orange?style=for-the-badge)]()
[![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)]()

<br>

<img src="https://imgs.search.brave.com/ugsQPwm10SNoXcVh75a42R-cvQz1AcJMKH7h070SP2E/rs:fit:860:0:0:0/g:ce/aHR0cHM6Ly9pbWFn/ZXMuc3F1YXJlc3Bh/Y2UtY2RuLmNvbS9j/b250ZW50L3YxLzU2/YzEzY2MwMDQ0MjYy/N2EwODYzMjk4OS83/ZjQzM2FjNi04OTM4/LTQ4NGUtYjIwNi02/YTlhNWJkZWM4Y2Mv/RnJleWphU29yY2Vy/ZXNzLnBuZw" alt="Freyja banner" width="600">

</div>

---

## ✦ What is Freyja?

Freyja is being built as **independent implementations of the same engine**, developed in parallel so each one keeps the others honest:

| | Language | API | Role |
|---|---|---|---|
| 🗡️ | [Odin](https://odin-lang.org/) | **Vulkan** | Primary engine — explicit, low-level, no hidden driver magic |
| 🪶 | [Odin](https://odin-lang.org/) | **OpenGL** | Odin-side reference backend — same language as the Vulkan engine, simpler API |
| 🛡️ | C++ | **OpenGL** | Reference / contribution backend — fast to prototype, easy to onboard into |

> The Vulkan side is where the real engine work happens. Both OpenGL implementations exist as simpler counterparts for testing ideas and sanity-checking rendering results before the same approach gets ported over to Vulkan — the Odin one keeps the language consistent with the primary engine, while the C++ one is a friendlier entry point for anyone contributing who doesn't want to wade through Vulkan boilerplate on day one.

---

## ✦ Repositories

Each backend lives in its own repository:

### 🗡️ [freyja](https://github.com/Amaterus1125/freyja)
**Odin + Vulkan** — primary engine

```bash
git clone https://github.com/Amaterus1125/Freyja.git
```

### 🪶 [freyja-odin-gl](https://github.com/Amaterus1125/freyja-odin-gl)
**Odin + OpenGL** — reference backend

```bash
git clone https://github.com/Amaterus1125/Freyja-odin-opengl.git
```

### 🛡️ [freyja-cpp-opengl](https://github.com/Amaterus1125/freyja-cpp-opengl)
**C++ + OpenGL** — contribution backend

```bash
git clone https://github.com/Shobhit-bit/Freyja-cpp.git
```

> Each repo also keeps its own branches for in-progress work and history (`main`, feature branches, etc.) — splitting into separate repos changes *where* each backend lives, not how branching works within it.

---

## ✦ Building

<table>
<tr>
<td width="33%" valign="top">

### 🗡️ freyja — Odin / Vulkan

Requires:
- [Odin compiler](https://odin-lang.org/docs/install/)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

```bash
odin run .
```

</td>
<td width="33%" valign="top">

### 🪶 freyja-odin-gl — Odin / OpenGL

Requires:
- [Odin compiler](https://odin-lang.org/docs/install/)
- (`vendor:glfw` / `vendor:OpenGL` ship with the compiler)

```bash
odin run .
```

</td>
<td width="34%" valign="top">

### 🛡️ freyja-cpp-opengl — C++ / OpenGL

Requires:
- CMake + a C++17 compiler
- GLAD generated once (see repo README)

```bash
cmake -B build
cmake --build build
```

</td>
</tr>
</table>

---

## ✦ Roadmap

- [ ] Core windowing + swapchain/context setup (all backends)
- [ ] Basic triangle rendering (all backends)
- [ ] Shared math / scene layer
- [ ] Model loading
- [ ] Lighting
- [ ] Feature-parity checkpoint across backends

---

## ✦ Why multiple backends?

Vulkan is explicit and verbose by design, excellent for learning exactly what the GPU is doing, but slow to iterate with. OpenGL trades that control for speed. Building both side by side keeps the Vulkan engine honest against a known-simple reference, while keeping iteration fast enough to actually experiment with ideas before committing them to the "real" backend.

---

<div align="center">

**License:** [MIT](LICENSE) &nbsp;•&nbsp; **Status:** actively in development, expect breaking changes

</div>
