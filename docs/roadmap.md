# Roadmap

This document contains the status, milestones, and development plan for Arunimesta. The roadmap is a direction, not a contract; priorities may change.

## Current Status

| Area | Status | Notes |
| :--- | :--- | :--- |
| Build system | ✅ Done | CMake ≥ 3.28, Clang, Ninja, Debug/Release presets |
| Named modules | ✅ Done | `aru` + `aru:application` partition, `lib.cppm` facade |
| `Application` | ✅ Basic | `std::expected` factory, `unique_ptr` pimpl, move-only |
| Windowing | 🚧 In progress | `aru.window` is still a skeleton; not in the root build |
| Vulkan renderer | ⬜ Not started | No RHI source yet |
| Testing | ⬜ Not started | The `test/` folder is still empty |
| Documentation | ✅ Initial | README + `docs/` |

## Milestones

### M0 — Foundation (done)

- [x] Modular workspace structure under `modules/`
- [x] C++23 named modules and facade
- [x] Clang x64 Debug/Release build presets
- [x] `Application` with `std::expected` and pimpl
- [x] Debug-only logging
- [x] Initial documentation

**Done criteria:** the project can be configured and built, and the `aru` executable runs and prints application info.

### M1 — Windowing (in progress)

- [ ] Finish the `Window` class in `aru.window:window`
- [ ] Fill in the `aru.window` facade (`lib.cppm`)
- [ ] Add `modules/aru_window` to the root `CMakeLists.txt`
- [ ] Basic event loop integration (polling)
- [ ] Create and destroy windows safely (RAII)
- [ ] Add a usage example in the executable

**Done criteria:** `aru` can create a window, process events, and exit cleanly.

### M2 — Vulkan RHI Foundation

- [ ] `aru_rhi` module (static library) and `aru.rhi` facade
- [ ] Vulkan loader/instance
- [ ] Debug messenger in debug builds
- [ ] Surface from the window
- [ ] Physical device and queue family selection
- [ ] Logical device and queue

**Done criteria:** the instance, surface, and device are created and destroyed without validation errors.

### M3 — Presentation

- [ ] Swapchain, image views, and framebuffers
- [ ] Render pass
- [ ] Format and present mode selection
- [ ] Resize handling (recreate swapchain)

**Done criteria:** an empty frame is successfully presented to the window.

### M4 — Frame Rendering

- [ ] Command pool and command buffers
- [ ] Synchronization (fence/semaphore) and frames in flight
- [ ] Graphics pipeline and shaders
- [ ] Vertex/index buffers
- [ ] Per-frame render loop

**Done criteria:** a simple object (for example a triangle) is displayed and animated.

### M5 — Engine Systems

- [ ] Resource management
- [ ] Math (for example GLM)
- [ ] ECS
- [ ] Asset pipeline
- [ ] Scene and camera
- [ ] PBR pipeline

## Design Direction

- **Module per subsystem**: `aru`, `aru.window`, `aru.rhi`, and other supporting modules.
- **Facade + partitions**: consumers import a single module name; the implementation is split internally.
- **`std::expected` in the public API**: explicit failures, not exceptions.
- **Pimpl + `unique_ptr`**: implementation details hidden, ownership clear.
- **RAII**: all resources are cleaned up by destructors.
- **Windows-first**: the current presets and targets are Windows x64; other platform support will follow.

## Technical Debt

Cleanup priorities before adding large subsystems:

1. Complete or temporarily remove the half-finished `modules/aru_window`.
2. Register `aru_window` in the root `CMakeLists.txt` once it is done.
3. Change `Application` copy assignment to `= delete`.
4. Replace the `bool` error type with an error enum/struct.
5. Add unit tests in `test/`.
6. Clean up the duplicate logs in `Application::init()`.
7. Add `operator new[] = delete` for consistency.
8. Add `.clang-format` if formatting should be automated.

## Non-Goals (For Now)

- Non-Windows platform support.
- Multi-threaded renderer.
- Visual editor.
- Custom asset format.

## How to Update the Roadmap

- Mark an item `[x]` only after the implementation is merged and the build succeeds.
- Move items whose priority changes to the appropriate milestone.
- Record new technical debt in the **Technical Debt** section.
- Update the [README](../README.md) if the summary status changes.
