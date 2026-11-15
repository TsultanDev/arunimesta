# Architecture

This document describes the current code structure of Arunimesta, the main design decisions, and the module boundaries.

## Overview

Arunimesta is a C++23 project that uses **named modules**. The code is split per subsystem under `modules/`, and each module has its own CMake target.

Currently there are only two modules:

```text
modules/
├── aru/            # executable + module aru
└── aru_window/     # static library aru.window (still a skeleton)
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
                │      :application        │
                └────────────┬─────────────┘
                             │
                ┌────────────▼─────────────┐
                │  module aru:application  │
                │  Application +           │
                │  Application_T           │
                └──────────────────────────┘

     (planned) module aru.window  ←── not used by aru yet
```

### Module `aru`

- `modules/aru/src/lib.cppm` is the **facade**:
  ```cpp
  export module aru;
  export import :application;
  ```
- `modules/aru/src/application.cppm` is the **partition** `aru:application`.
- `main.cpp` only needs to do `import aru;` and does not need to know about internal partitions.

The facade/partition split is useful as the module grows: consumers keep importing a single name (`aru`), while the implementation is split into multiple partitions.

### Module `aru.window` (work in progress)

- The static library target `aru_window` already has a `CMakeLists.txt`.
- `src/window.cppm` only contains the skeleton `export module aru.window:window;` and incomplete class declarations.
- `src/lib.cppm` is still empty.
- This module has **not** been added to `add_subdirectory()` in the root `CMakeLists.txt`, so it is not built.

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
Application& operator=(const Application&);   // declared, not defined
Application(Application&&) noexcept = default;
Application& operator=(Application&&) noexcept = default;
void* operator new(size_t) = delete;
```

- Objects can be moved, but not copied.
- `operator new` is deleted so `Application` cannot be created with plain `new`; creation only happens through `Application::init()`.
- Note: copy assignment is still **declared without a definition**. As long as it is not used, the build succeeds; this needs to be cleaned up (explicitly deleted) in the next API cleanup.

### Lifetime

```text
main()
  └─ Application::init(name, version)
       ├─ make_unique<Application_T>
       ├─ (debug) println log
       └─ return std::expected<Application, bool>
            └─ printInfo()
                 └─ destructor Application → unique_ptr → ~Application_T
```

There is no `Application::run()` in this iteration of the code yet. `main.cpp` only calls `printInfo()` inside `transform`.

## Error Handling

- The public API uses `std::expected`.
- No exceptions are thrown by engine code yet.
- The error type is currently `bool`, so there is no information about the cause of failure.
- Future recommendation: replace it with an `enum class Error` or an error struct that can provide context.

## Logging

- Logging uses `std::println` and is only active in debug builds with `#ifndef NDEBUG`.
- The prefix currently used is: `Aru: Application(...)`.
- Target convention: `Aru: <Area>(<event>)`, for example `Aru: Window(created!!)`.
- Currently `init()` prints two similar log lines; this can be tidied up later.

## Build and Module Boundaries

- The root `CMakeLists.txt` only adds `modules/aru`.
- Each module registers its `.cppm` files through `FILE_SET CXX_MODULES`.
- Standard headers are included in the **global module fragment** (`module;` ... `export module ...;`) so they do not leak to consumers.
- `aru` does not depend on `aru.window` at this time.

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
| `aru.window` is incomplete | Windowing cannot be used yet | Finish the `Window` class, fill in the facade, add it to the root CMake |
| `aru_window` is not in `add_subdirectory` | The module is not built | Add it once the source is complete |
| `Application` copy assignment has no definition | Potential link error if used | Delete it explicitly (`= delete`) |
| `bool` error type | No failure details | Replace with an error enum/struct |
| No tests yet | Regressions go undetected | Fill the `test/` folder with unit tests |
| Duplicate logs in `init()` | Repeated debug output | Clean up during refactor |
