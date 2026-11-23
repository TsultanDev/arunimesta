# Architecture

This document describes the current code structure of Arunimesta, the main design decisions, and the module boundaries.

## Overview

Arunimesta is a C++23 project that uses **named modules**. Each subsystem is a top-level subproject at the repository root with its own CMake target.

Currently there are three subprojects:

```text
./
├── aru/            # executable + module aru (facade)
├── aru_window/     # static library aru.window (Win32 windowing)
└── aru_rhi/        # static library aru.rhi (Vulkan presentation foundation)
```

All three targets are active. The root `CMakeLists.txt` registers `aru_window`, `aru_rhi`, and `aru`; the `aru` facade re-exports `aru.window` and `aru.rhi`. Standard-library types (containers, strings, error wrappers, printing) are used directly by engine code instead of a project-specific standard-library module.

The Vulkan renderer (`aru.rhi`) provides the presentation foundation. The remaining rendering work is tracked in [roadmap.md](roadmap.md).

## Module Map

```text
            ┌──────────────────────────┐
            │        main.cpp          │
            │       import aru;        │
            └────────────┬─────────────┘
                         │
            ┌────────────▼─────────────┐
            │      module aru          │
            │      (lib.cppm)          │
            │   export import          │
            │   aru.window, aru.rhi    │
            └───┬──────────────┬───────┘
                │              │
  ┌─────────────▼────────┐  ┌──▼─────────────────────────────────────────┐
  │ module aru.window    │  │ module aru.rhi                              │
  │ :error, :window      │  │ :error :instance :surface :adapter :device  │
  │ Window (Win32)       │  │ :frame :queue :resource :swapchain          │
  │                      │  │ :render_pass :framebuffer :command_pool    │
  │                      │  │ :command_buffer                            │
  └──────────────────────┘  └─────────────────────────────────────────────┘
```

### Module `aru`

- `aru/src/lib.cppm` is the **facade**:
  ```cpp
  export module aru;
  export import aru.window;
  export import aru.rhi;
  ```
- `main.cpp` only needs to do `import aru;` and gets `aru.window` plus `aru.rhi`.
- The executable links both static libraries: `target_link_libraries(aru PRIVATE aru_window aru_rhi)`.

The facade keeps the public import surface small: consumers import a single name (`aru`) and receive both engine modules.

### Module `aru.rhi` (presentation foundation)

- The static library target `aru_rhi` is registered in the root `CMakeLists.txt` and links `Vulkan::Headers` and `aru_window`.
- `src/lib.cppm` is the facade:
  ```cpp
  export module aru.rhi;
  export import :error;
  export import :instance;
  export import :surface;
  export import :adapter;
  export import :device;
  export import :frame;
  export import :queue;
  export import :resource;
  export import :swapchain;
  export import :render_pass;
  export import :framebuffer;
  export import :command_pool;
  export import :command_buffer;
  ```
- `src/error.cppm` is the partition `aru.rhi:error`. It owns the formattable `Error` enum; other RHI partitions `export import :error;`.
- `src/instance.cppm` is the partition `aru.rhi:instance`. It exports `QueueFlags` with `constexpr` bitwise operators and `Instance`, and enables `VK_KHR_surface` + `VK_KHR_win32_surface`.
- `src/surface.cppm` is the partition `aru.rhi:surface`. It exports `Surface` and defines `Instance::createSurface(const Window&)`; it imports `:instance` and `aru.window`.
- `src/adapter.cppm` is the partition `aru.rhi:adapter`. It exports `Adapter` and defines `Instance::requestAdapter(const Surface&)`; it selects the first graphics + present queue family.
- `src/device.cppm` is the partition `aru.rhi:device`. It exports `Device`, defines `Adapter::createDevice(const Adapter&)` and `Device::acquire(callback)`, and caches swapchain/frame/sync state; it enables `VK_KHR_swapchain`.
- `src/frame.cppm` is the partition `aru.rhi:frame`. It defines the `Device` frame helpers: lazy fence/semaphore creation, fence wait/reset, image acquisition, image/frame observers, and frame index rollover.
- `src/queue.cppm` is the partition `aru.rhi:queue`. It exports `Queue` and defines `Device::requestQueue(QueueFlags)`, validating flags with the `constexpr` bitwise helpers; `submit()`/`present()` are defined in `:command_buffer`.
- `src/resource.cppm` is the partition `aru.rhi:resource`. It exports `Resource` and `ResourceKind`; `Resource` is a tagged union of `ImageData { image, view }` and `BufferData { buffer, view }`, with an ownership flag for safe copies.
- `src/swapchain.cppm` is the partition `aru.rhi:swapchain`. It exports `Swapchain` and defines `Device::createSwapchain(width, height)` and `Device::createSwapchainImages(swapchain)`.
- `src/render_pass.cppm` is the partition `aru.rhi:render_pass`. It exports `RenderPass`, defines `Device::createRenderPass(swapchain)`, and implements `render(callback)` / `render(clearColor, callback)` around `vkCmdBeginRenderPass` / `vkCmdEndRenderPass`.
- `src/framebuffer.cppm` is the partition `aru.rhi:framebuffer`. It exports `Framebuffer` and defines `Device::createFramebuffer(renderPass, image)`.
- `src/command_pool.cppm` is the partition `aru.rhi:command_pool`. It exports `CommandPool` and defines `Device::createCommandPool(queue)`; pools are created with `eResetCommandBuffer` for the queue's family.
- `src/command_buffer.cppm` is the partition `aru.rhi:command_buffer`. It exports `CommandBuffer`, defines `Device::allocateCommandBuffers(pool, count)`, implements `record(callback)` (reset + begin + callback + end), and defines `Queue::submit(buffer)` / `Queue::present()` with the frame semaphores.
- `Instance::create(std::string_view appName)` takes a UTF-8 application name and returns `std::expected<Instance, Error>`; `applicationName()` returns `const std::string&`.
- All RHI factories return `std::expected<T, Error>`; `Device::createSwapchainImages()` returns `std::expected<std::vector<Resource>, Error>` and `Device::allocateCommandBuffers()` returns `std::expected<std::vector<CommandBuffer>, Error>`.
- The API version comes from `vkEnumerateInstanceVersion`, not from the caller; Vulkan 1.0 falls back to `VK_API_VERSION_1_0`.
- `aru_rhi/vulkan.hpp` also defines `VK_USE_PLATFORM_WIN32_KHR` for every translation unit so the dynamic dispatcher has one consistent layout.
- Vulkan-Hpp uses **dynamic dispatch** (`VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1`). The dispatcher storage is defined exactly once in `src/instance.cppm`; other translation units include `aru_rhi/vulkan.hpp`, which only declares it `extern`.
- Rule of Five: copy duplicates metadata but not the owned Vulkan handle; move steals the handle and empties the source. Destructors destroy only owned handles (`Instance`, `Surface`, `Device`, `Swapchain`, `RenderPass`, `Framebuffer`, and owned `Resource` views/buffers); `Adapter` and `Queue` are non-owning views. A copied `Resource` keeps its handles but is marked non-owning to avoid double destruction.
- Setup order in `main.cpp`: window → instance → surface → adapter → device → queue → swapchain → swapchain images → render pass → framebuffers → command pool → command buffers. Reverse declaration order destroys buffers before their pool.
- Frame order in `main.cpp`: `Window::pollEvents()` → `Device::acquire(callback)` → `CommandBuffer::record(callback)` → `RenderPass::render(callback)` → `Queue::submit(buffer)` → `Queue::present()`. See [api/frame.md](api/frame.md).

### Module `aru.window` (active)

- The static library target `aru_window` has a `CMakeLists.txt`, is registered in the root `CMakeLists.txt`, and is linked into the `aru` executable.
- `src/window.cppm` is the partition `aru.window:window`. It implements `Window::create()` (both overloads), `createFromWindow()`, `copy()`, `move()`, and `event_loop()`.
- `src/lib.cppm` is the facade and re-exports the partitions:
  ```cpp
  export module aru.window;
  export import :error;
  export import :window;
  ```
- `src/error.cppm` is the partition `aru.window:error`. It owns the formattable `Error` enum; `window.cppm` re-exports it.
- `aru/src/lib.cppm` re-exports `aru.window`, so consumers can use it through `import aru;`.
- The implementation uses the Win32 Unicode APIs: `RegisterClassW`, `CreateWindowExW`, `DefWindowProcW`, `GetMessageW`, and `DispatchMessageW`.
- `Window::ResultType` is `std::expected<Window, Error>`. `create(const char*, uint32_t, uint32_t)` treats a narrow title as UTF-8 and converts it with `MultiByteToWideChar(CP_UTF8, ...)`; a `create(const char16_t*, uint32_t, uint32_t)` overload also exists, and `windowTitle()` returns `const std::wstring&`. `Error` is formattable, so `std::println("{}", err)` works.
- Rule of Five: the copy constructor / copy assignment / `copy()` duplicate the title and metadata but not the `HWND`; the move constructor / move assignment / `move()` steal the handle and empty the source. `createFromWindow()` creates a genuinely new OS window.
- The window class name is `ArunimestaWindowClass`; registration is idempotent, so `ERROR_CLASS_ALREADY_EXISTS` is ignored. The destructor calls `DestroyWindow` only when `hwnd != nullptr`; the `std::wstring` title is RAII-managed.

### Ownership and Rule of Five

Engine-owned types (`Window`, `Surface`, `Adapter`, `Device`, `Queue`, `Resource`, `Swapchain`, `RenderPass`, `Framebuffer`, `CommandPool`, `CommandBuffer`) follow the Rule of Five with explicit `copy()` / `move()` methods:

- Copy duplicates metadata but not the owned handle or `HWND`; move steals the handle and empties the source, so destructors destroy only owned handles.
- Pass these objects to containers through `move()` when handle ownership must transfer, for example `framebuffers.push_back(framebuffer.move())` with a `std::vector<Framebuffer>`.
- This convention is specific to engine-owned resource wrappers; containers, strings, and error wrappers come from the C++ standard library.

### Unicode and String Handling

- The public API takes UTF-8 text. With Clang, the source and execution character sets are UTF-8.
- Text uses standard-library types: `std::string`, `std::u8string`, `std::u16string`, `std::u32string`, `std::wstring`, and `std::string_view`. There is no project-specific string type.
- Win32 `...W` APIs take UTF-16. `Window::create(const char*, ...)` converts a UTF-8 title with `MultiByteToWideChar(CP_UTF8, ...)` before calling `CreateWindowExW`; `windowTitle()` exposes the stored `const std::wstring&`. There is also a direct `create(const char16_t*, ...)` overload.
- `UNICODE` and `_UNICODE` are defined for the `aru_window` target in `aru_window/CMakeLists.txt`.
- **Never mix ANSI (`...A`) and Unicode (`...W`) Win32 APIs for the same window.** Doing so corrupts text: an earlier version created the window with `CreateWindowExA`, but the window procedure called `DefWindowProcW`. `DefWindowProcW` re-read the ANSI title pointer as UTF-16, so the bytes of `"Arunimesta"` were displayed as CJK characters (`U+7241 U+6E75 ...`). Using the `W` APIs consistently fixes this.

## `Application` Design Patterns

### Pimpl with `unique_ptr`

The implementation details of `Application` are separated into the `Application_T` struct:

```cpp
namespace aru {
    struct Application_T;
    export class Application;
}

struct aru::Application_T {
    std::string name = "";
    uint32_t version = 0;
};

class aru::Application {
    // ...
private:
    Application(std::unique_ptr<Application_T> app);
    std::unique_ptr<Application_T> app;
};
```

Consequences:

- The public layout of `Application` stays stable even if internal fields change.
- `Application_T` is not exposed to module consumers.
- `Application_T` is destroyed automatically through `unique_ptr` (RAII).

### Factory and `std::expected`

Objects are not created with a public constructor, but through a factory:

```cpp
static std::expected<Application, ApplicationError> init(std::string name, uint32_t version);
```

- No exceptions are thrown for initialization failures.
- Callers must check the result (`if (!app)` or `.transform(...)`).
- The error type is `ApplicationError`; it currently only has `Unknown`, and `init()` always succeeds. The enum can grow as failure paths appear.

### Move-only and heap allocation ban

```cpp
Application(const Application&) = delete;
Application& operator=(const Application&) = delete;
Application(Application&&) noexcept = default;
Application& operator=(Application&&) noexcept = default;
void* operator new(size_t) = delete;
```

- Objects can be moved, but not copied.
- `operator new` is deleted so `Application` cannot be created with plain `new`; creation only happens through `Application::init()`.
- Only the scalar `operator new` is deleted; `operator new[]` should be added for consistency.

### Lifetime

```text
main()
  ├─ Application::init(name, version)          (currently unused by main.cpp)
  │    ├─ make_unique<Application_T>
  │    ├─ (debug) std::println log
  │    └─ return std::expected<Application, ApplicationError>
  └─ current main.cpp:
       └─ Window::create("Arunimesta", 800, 600) → std::expected<Window, window::Error>
            ├─ check the expected result (report errors with std::println)
            ├─ UTF-8 → UTF-16 via MultiByteToWideChar(CP_UTF8, ...)
            └─ CreateWindowExW + ShowWindow
                 └─ window.event_loop()
                      └─ WM_DESTROY → PostQuitMessage → WM_QUIT → loop ends
```

There is no `Application::run()` in this iteration yet. `main.cpp` currently exercises the window creation + `std::expected` flow; `Application` remains available through `aru:application` but is not imported by the facade.

## Error Handling

- The public API uses `std::expected` together with `std::unexpected(...)` for failures.
- No exceptions are thrown by engine code yet.
- `Application` uses `ApplicationError` (currently only `Unknown`).
- `std::optional<T>` is used when a value may be absent; prefer the standard library over custom wrappers.
- Future recommendation: grow the error enums/structs so they provide context.

## Logging

- Logging uses `std::println` and is only active in debug builds with `#ifndef NDEBUG`.
- The prefix currently used is: `Aru: Application(...)` and `Aru: Window(created!!), title: ...`.
- Target convention: `Aru: <Area>(<event>)`, for example `Aru: Window(created!!)`.
- Currently `init()` prints two similar log lines; this can be tidied up later.

## Build and Module Boundaries

- The root `CMakeLists.txt` adds `aru_window`, `aru_rhi`, and `aru`.
- Each module registers its `.cppm` files through `FILE_SET CXX_MODULES`.
- Standard headers are included in the **global module fragment** (`module;` ... `export module ...;`) so they do not leak to consumers.
- `aru` depends on both libraries: `target_link_libraries(aru PRIVATE aru_window aru_rhi)`.
- The `aru` facade re-exports `aru.window` and `aru.rhi`, so `import aru;` provides `Window`, `Instance`, `Surface`, `Adapter`, `Device`, `Queue`, `Resource`, `Swapchain`, `RenderPass`, and `Framebuffer`. Standard-library types come from headers such as `<expected>`, `<optional>`, `<string>`, `<string_view>`, `<vector>`, `<array>`, and `<print>`.

## Adding a New Module

General steps:

1. Create the top-level directory `<name>/src`.
2. Add a partition file, for example `foo.cppm` with `export module aru:foo;`.
3. Add a facade `lib.cppm` with `export module aru; export import :foo;` (or a separate module if it is not part of `aru`).
4. Create the module's `CMakeLists.txt` with `add_library(...)` and `target_sources(... FILE_SET CXX_MODULES ...)`.
5. Register the module in the root `CMakeLists.txt` with `add_subdirectory(...)`.
6. If the module is used by an executable, add the target dependency in `target_link_libraries`.

Define member and free functions directly in the partition that owns them. A declaration without a body is reserved for a member of a class defined in another partition that needs a complete type here; use forward declarations only to break import cycles.

## Known Technical Debt

| Item | Impact | Plan |
| :--- | :--- | :--- |
| `Window` copy does not duplicate the OS window | `copy()` / copy assignment copy metadata only; `handle()` is null on the copy | Use `createFromWindow()` when a real second window is needed |
| `event_loop()` blocks | It uses `GetMessageW` until `WM_QUIT`, so it is not used by the render loop | `pollEvents()` exists for per-frame message pumping |
| No input handling | Keyboard and mouse messages are ignored | Handle `WM_KEYDOWN` / `WM_MOUSEMOVE` and expose events |
| Empty frame only | The frame loop submits and presents a cleared frame; no draw commands, pipeline, or resize handling | Add a graphics pipeline and per-frame draw recording |
| No resize handling | `WM_SIZE` is not processed | Handle resize before the RHI needs it |
| `ApplicationError` is minimal | Only `Unknown`; no failure context | Expand the error enum/struct as failure paths appear |
| No tests yet | Regressions go undetected | Fill the `test/` folder with unit tests |
| Duplicate logs in `init()` | Repeated debug output | Clean up during refactor |
