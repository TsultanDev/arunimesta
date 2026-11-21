# Roadmap

This document contains the status, milestones, and development plan for Arunimesta. The roadmap is a direction, not a contract; priorities may change.

## Current Status

| Area | Status | Notes |
| :--- | :--- | :--- |
| Build system | ✅ Done | CMake ≥ 3.28, Clang, Ninja, Debug/Release presets |
| Named modules | ✅ Done | `aru` + `aru:application` partition, `lib.cppm` facade |
| `Application` | ✅ Basic | `std::expected` factory, `unique_ptr` pimpl, move-only |
| C++ standard library | ✅ In use | `std::expected` / `std::optional`, `std::string` family, `std::string_view`, `std::vector` / `std::array`, `std::println` / `std::eprintln` used directly |
| Windowing | ✅ Basic | `aru.window` active: Win32 `Window`, `std::expected` errors, copy/move assignment, event loop |
| Vulkan renderer (RHI) | ✅ Basic | `aru.rhi` active: dynamic-dispatch Vulkan-Hpp `Instance` from `std::string_view`, Win32 `Surface`, `Adapter`, `Device`, `Queue`, `Swapchain`, `Resource` (image/buffer tagged union), `RenderPass`, `Framebuffer`, `CommandPool`, `CommandBuffer`, and an empty-frame loop with fences/semaphores |
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

### M0.5 — C++ Standard Library Migration (done)

- [x] Removed the custom standard-library module and its CMake target
- [x] Fallible operations return `std::expected<T, E>` built with `std::unexpected(...)`
- [x] Absent values use `std::optional<T>`
- [x] Text uses `std::string`, `std::u8string`, `std::u16string`, `std::u32string`, `std::wstring`, and `std::string_view`
- [x] Containers use `std::vector<T>` and `std::array<T, N>`
- [x] Logging uses `std::println` / `std::eprintln`
- [x] `std::formatter` specializations for `aru::window::Error` and `aru::rhi::Error`

**Done criteria:** engine code uses the C++ standard library directly, the `aru` facade re-exports only `aru.window` and `aru.rhi`, and the project builds.

### M1 — Windowing (done; basic)

- [x] Finish the `Window` class in `aru.window:window`
- [x] Safe copy/move assignment plus `copy()` / `move()`; full Rule of Five (copy/move constructors implemented)
- [x] Fill in the `aru.window` facade (`lib.cppm`)
- [x] Register `modules/aru_window` in the root `CMakeLists.txt` and re-export `aru.window`
- [x] Blocking event loop integration (`GetMessageW`)
- [x] Unicode window titles (UTF-8 → UTF-16 via `MultiByteToWideChar(CP_UTF8, ...)`)
- [x] Create and destroy windows safely (RAII with null checks)
- [x] `createFromWindow()` for a genuinely new OS window
- [x] Add a usage example in the executable
- [ ] Handle keyboard and mouse input

**Done criteria:** `aru` can create a window, process events, and exit cleanly. The module is active in the build and exported through `import aru;`.

### M2 — Vulkan RHI Foundation

- [x] `aru_rhi` module (static library) and `aru.rhi` facade
- [x] Vulkan loader/instance (dynamic dispatch, `vkEnumerateInstanceVersion`)
- [ ] Debug messenger in debug builds
- [x] Surface from the window (`Instance::createSurface(Window)`)
- [x] Physical device and queue family selection (`Instance::requestAdapter(Surface)`)
- [x] Logical device and queue (`Adapter::createDevice()`, `Device::requestQueue(QueueFlags)` with `constexpr` bitwise operators)

**Done criteria:** the instance, surface, and device are created and destroyed without validation errors.

### M3 — Presentation (foundation done)

- [x] Swapchain creation with format and present mode selection (`Device::createSwapchain`)
- [x] Swapchain images with owned image views (`Device::createSwapchainImages`, returning `std::expected<std::vector<Resource>, Error>`)
- [x] `Resource` tagged union for image + view / buffer + view
- [x] Render pass with a single color attachment (`Device::createRenderPass`)
- [x] One framebuffer per swapchain image (`Device::createFramebuffer`)
- [x] Command pool and per-image command buffers (`Device::createCommandPool`, `Device::allocateCommandBuffers`)
- [x] Frame loop prototype (`acquire` → `record` → `render` → `submit` → `present`) with fence/semaphore synchronization and two frames in flight
- [x] Presenting an empty cleared frame (`Queue::submit`, `Queue::present`)
- [ ] Resize handling (recreate swapchain)

**Done criteria:** an empty frame is successfully presented to the window.

### M4 — Frame Rendering

- [ ] Recording draw commands into command buffers
- [ ] Graphics pipeline and shaders
- [ ] Vertex/index buffers
- [ ] Expand the prototype into a full per-frame render loop with resource updates

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
- **Standard library first**: use `std::vector`, `std::array`, the `std::string` family, `std::optional`, and `std::println` directly instead of custom wrappers.
- **Pimpl + `unique_ptr`**: implementation details hidden, ownership clear.
- **RAII**: all resources are cleaned up by destructors; engine-owned wrappers follow the Rule of Five with explicit `copy()` / `move()` and transfer ownership with `move()` when stored in containers.
- **UTF-aware text**: text uses `std::string` and the UTF `std::u8string` / `std::u16string` / `std::u32string` variants, viewed via `std::string_view`; UTF-8 titles are converted to UTF-16 with `MultiByteToWideChar(CP_UTF8, ...)` at the Win32 boundary.
- **Unicode-first Win32**: use the `...W` APIs and convert UTF-8 titles to UTF-16 before calling Win32.
- **Windows-first**: the current presets and targets are Windows x64; other platform support will follow.

## Technical Debt

Cleanup priorities before adding large subsystems:

1. Handle keyboard and mouse input.
2. Add a non-blocking/polling event-loop variant if the renderer needs one.
3. Carry the Win32 error code in `aru::window::Error`.
4. Handle resize (`WM_SIZE`).
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
