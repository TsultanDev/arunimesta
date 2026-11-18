# Roadmap

This document contains the status, milestones, and development plan for Arunimesta. The roadmap is a direction, not a contract; priorities may change.

## Current Status

| Area | Status | Notes |
| :--- | :--- | :--- |
| Build system | ✅ Done | CMake ≥ 3.28, Clang, Ninja, Debug/Release presets |
| Named modules | ✅ Done | `aru` + `aru:application` partition, `lib.cppm` facade |
| `Application` | ✅ Basic | `std::expected` factory, `unique_ptr` pimpl, move-only |
| `aru.standard` (`String`) | ✅ Basic | `char8_t`/`char16_t`/`char32_t`, transcoding `from()`, `move()`, `U+FFFD` replacement |
| `aru.standard` (`Vector`) | ✅ Basic | Factory-only allocation (`create()`), `copy()`/`move()`, `iter()`/`intoIter()` |
| `aru.standard` (`Result`) | ✅ Basic | Discriminated union, in-place construction, `valueOrUnexpected()` |
| `aru.standard` (`print`) | ✅ Basic | Debug-gated `println` / `eprintln` |
| Windowing | ✅ Basic | `aru.window` active: Win32 `Window`, `Result` errors, copy/move assignment, event loop |
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

### M0.5 — `aru.standard` Foundation (basic implementation done)

- [x] `aru_std` static library and `aru.standard` facade
- [x] `:print` partition — debug-gated `println` / `eprintln`
- [x] `:result` partition — `Result<T, E>` discriminated union
- [x] `:string` partition — `String<T>` for `char8_t`/`char16_t`/`char32_t`
- [x] Cross-encoding conversion via static `String::from()` factory (UTF-8 ⇄ UTF-16 ⇄ UTF-32, `U+FFFD` on invalid input)
- [x] `:vector` partition — `Vector<T>` with factory-only allocation (`create()`), `copy()`/`move()`, initializer-list assignment, `iter()`/`intoIter()`
- [x] Factory-only allocation policy for `String` and `Vector`; copy/move constructors deleted
- [ ] String concatenation (`append`, `+`, `+=`)
- [ ] `std::formatter` / `std::hash` support for `String` and `Vector`

**Done criteria:** strings in the three UTF encodings and `Vector<T>` can be allocated through their factories, copied/moved through assignment or `copy()`/`move()`, and converted without data loss; `aru` links `aru_std` and re-exports it through `import aru;`.

### M1 — Windowing (done; basic)

- [x] Finish the `Window` class in `aru.window:window`
- [x] Safe copy/move assignment plus `copy()` / `move()`; copy/move constructors deleted
- [x] Fill in the `aru.window` facade (`lib.cppm`)
- [x] Register `modules/aru_window` in the root `CMakeLists.txt` and re-export `aru.window`
- [x] Blocking event loop integration (`GetMessageW`)
- [x] Unicode window titles (UTF-8 → UTF-16 via `String`)
- [x] Create and destroy windows safely (RAII with null checks)
- [x] `createFromWindow()` for a genuinely new OS window
- [x] Add a usage example in the executable
- [ ] Handle keyboard and mouse input

**Done criteria:** `aru` can create a window, process events, and exit cleanly. The module is active in the build and exported through `import aru;`.

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

- **Module per subsystem**: `aru`, `aru.standard`, `aru.window`, `aru.rhi`, and other supporting modules.
- **Facade + partitions**: consumers import a single module name; the implementation is split internally.
- **`std::expected` in the public API**: explicit failures, not exceptions.
- **Pimpl + `unique_ptr`**: implementation details hidden, ownership clear.
- **RAII**: all resources are cleaned up by destructors.
- **UTF-aware strings**: text moves through `aru::standard::String` (`char8_t`/`char16_t`/`char32_t`); conversion happens only when a *new* string is created with `String::from()`.
- **Unicode-first Win32**: use the `...W` APIs and convert UTF-8 titles to UTF-16 before calling Win32.
- **Windows-first**: the current presets and targets are Windows x64; other platform support will follow.

## Technical Debt

Cleanup priorities before adding large subsystems:

1. Handle keyboard and mouse input.
2. Add a non-blocking/polling event-loop variant if the renderer needs one.
3. Carry the Win32 error code in `aru::window::Error`.
4. Handle resize (`WM_SIZE`).
5. Add `String` concatenation (`append`, `+`, `+=`).
6. Add `std::formatter` / `std::hash` support for `String`.
7. Add unit tests in `test/`.
8. Clean up the duplicate logs in `Application::init()`.
9. Add `operator new[] = delete` for consistency.
10. Add `.clang-format` if formatting should be automated.

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
