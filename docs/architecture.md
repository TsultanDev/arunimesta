# Architecture

This document describes the current code structure of Arunimesta, the main design decisions, and the module boundaries.

## Overview

Arunimesta is a C++23 project that uses **named modules**. The code is split per subsystem under `modules/`, and each module has its own CMake target.

Currently there are only two modules:

```text
modules/
├── aru/            # executable + module aru
└── aru_window/     # static library aru.window (Win32 windowing)
```

The Vulkan renderer **does not exist yet** in the repository. Its planned architecture is discussed in [roadmap.md](roadmap.md).

## Module Map

```text
                ┌──────────────────────────┐
                │        main.cpp          │
                │  import aru;             │
                └────────────┬─────────────┘
                             │
                ┌────────────▼─────────────┐
                │      module aru          │
                │      (lib.cppm)          │
                │  export import           │
                │  :application, aru.window│
                └───────┬──────────┬───────┘
                        │          │
        ┌───────────────▼──┐   ┌───▼──────────────────┐
        │ module           │   │ module aru.window    │
        │ aru:application  │   │ Window::create_window│
        │ Application +    │   │ Window::event_loop   │
        │ Application_T    │   └──────────────────────┘
        └──────────────────┘
```

### Module `aru`

- `modules/aru/src/lib.cppm` is the **facade**:
  ```cpp
  export module aru;
  export import :application;

  export import aru.window;
  ```
- `modules/aru/src/application.cppm` is the **partition** `aru:application`.
- `main.cpp` only needs to do `import aru;` and does not need to know about internal partitions.

The facade/partition split is useful as the module grows: consumers keep importing a single name (`aru`), while the implementation is split into multiple partitions.

### Module `aru.window`

- The static library target `aru_window` has a `CMakeLists.txt`, is registered in the root `CMakeLists.txt`, and is linked into the `aru` executable.
- `src/window.cppm` is the partition `aru.window:window`. It implements `Window::create_window()` and `Window::event_loop()`.
- `src/lib.cppm` is the facade and re-exports the partition:
  ```cpp
  export module aru.window;
  export import :window;
  ```
- `modules/aru/src/lib.cppm` re-exports `aru.window`, so consumers can use it through `import aru;`.
- The implementation uses the Win32 Unicode APIs: `RegisterClassExW`, `CreateWindowExW`, `DefWindowProcW`, `PeekMessageW`, and `DispatchMessageW`.
- The window class name is `ArunimestaClass`; registration is idempotent, so `ERROR_CLASS_ALREADY_EXISTS` is ignored.

### Unicode and String Handling

- The public API takes UTF-8 `std::string` values. With Clang, the source and execution character sets are UTF-8.
- Win32 `...W` APIs take UTF-16. The internal `toWide()` helper converts titles with `MultiByteToWideChar(CP_UTF8, ...)` before calling `CreateWindowExW`.
- `UNICODE` and `_UNICODE` are defined for the `aru_window` target in `modules/aru_window/CMakeLists.txt`.
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
static std::expected<Application, bool> init(std::string name, uint32_t version);
```

- No exceptions are thrown for initialization failures.
- Callers must check the result (`if (!app)` or `.transform(...)`).
- The second `bool` parameter is currently a placeholder error type. No failure values are returned yet; `init()` always succeeds. This type can be replaced with an error enum/struct in the future.

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
  └─ Application::init(name, version)
       ├─ make_unique<Application_T>
       ├─ (debug) println log
       └─ return std::expected<Application, bool>
            ├─ printInfo()
            └─ window::Window::create_window(title, width, height)
                 ├─ toWide(title)  (UTF-8 → UTF-16)
                 ├─ CreateWindowExW + ShowWindow
                 └─ while (window.event_loop()) { ... }
                      └─ WM_DESTROY → PostQuitMessage → WM_QUIT → return false
```

There is no `Application::run()` in this iteration of the code yet. `main.cpp` calls `printInfo()` and then creates the window inside `transform`.

## Error Handling

- The public API uses `std::expected`.
- No exceptions are thrown by engine code yet.
- The error type is currently `bool`, so there is no information about the cause of failure.
- Future recommendation: replace it with an `enum class Error` or an error struct that can provide context.

## Logging

- Logging uses `std::println` and is only active in debug builds with `#ifndef NDEBUG`.
- The prefix currently used is: `Aru: Application(...)` and `Aru: Window(created!!), title: ...`.
- Target convention: `Aru: <Area>(<event>)`, for example `Aru: Window(created!!)`.
- Currently `init()` prints two similar log lines; this can be tidied up later.

## Build and Module Boundaries

- The root `CMakeLists.txt` adds `modules/aru` and `modules/aru_window`.
- Each module registers its `.cppm` files through `FILE_SET CXX_MODULES`.
- Standard headers are included in the **global module fragment** (`module;` ... `export module ...;`) so they do not leak to consumers.
- `aru` depends on `aru_window` through `target_link_libraries(aru PRIVATE aru_window)`.

## Adding a New Module

General steps:

1. Create the directory `modules/<name>/src`.
2. Add a partition file, for example `foo.cppm` with `export module aru:foo;`.
3. Add a facade `lib.cppm` with `export module aru; export import :foo;` (or a separate module if it is not part of `aru`).
4. Create the module's `CMakeLists.txt` with `add_library(...)` and `target_sources(... FILE_SET CXX_MODULES ...)`.
5. Register the module in the root `CMakeLists.txt` with `add_subdirectory(...)`.
6. If the module is used by an executable, add the target dependency in `target_link_libraries`.

## Known Technical Debt

| Item | Impact | Plan |
| :--- | :--- | :--- |
| Window teardown is implicit | `DestroyWindow` is not called; cleanup relies on process exit | Add RAII teardown in `Window_T` or `Window` |
| No input handling | Keyboard and mouse messages are ignored | Handle `WM_KEYDOWN` / `WM_MOUSEMOVE` and expose events |
| No resize handling | `WM_SIZE` is not processed | Handle resize before the RHI needs it |
| `bool` error type | No failure details | Replace with an error enum/struct |
| No tests yet | Regressions go undetected | Fill the `test/` folder with unit tests |
| Duplicate logs in `init()` | Repeated debug output | Clean up during refactor |
