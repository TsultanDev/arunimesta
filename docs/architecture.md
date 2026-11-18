# Architecture

This document describes the current code structure of Arunimesta, the main design decisions, and the module boundaries.

## Overview

Arunimesta is a C++23 project that uses **named modules**. The code is split per subsystem under `modules/`, and each module has its own CMake target.

Currently there are three modules:

```text
modules/
├── aru/            # executable + module aru
├── aru_std/        # static library aru.standard (String, Result, print)
└── aru_window/     # static library aru.window (Win32 windowing)
```

The `aru_window` target is **temporarily disabled in the build** (commented out in the root `CMakeLists.txt` and in the `aru` facade) while the `Window` class is being refactored. The source and its reference documentation are kept in the repository; re-enable it by restoring the `add_subdirectory` line and the `export import aru.window;` line.

The Vulkan renderer **does not exist yet** in the repository. Its planned architecture is discussed in [roadmap.md](roadmap.md).

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
            │   :application,          │
            │   aru.standard           │
            └───────┬──────────┬───────┘
                    │          │
    ┌───────────────▼──────┐  ┌▼──────────────────────────────────────────┐
    │ module               │  │ module aru.standard                       │
    │ aru:application      │  │ :string      :result    :print   :vector  │
    │ Application +        │  │ String<T>    Result<T,E>         Vector<T>│
    │ Application_T        │  │                     println  copy/move    │
    └──────────────────────┘  └───────────────────────────────────────────┘

    module aru.window (paused; disabled in build)
    └── :window — Window::create_window, Window::event_loop
```

### Module `aru`

- `modules/aru/src/lib.cppm` is the **facade**:
  ```cpp
  export module aru;
  export import :application;
  export import aru.standard;

  // export import aru.window;  // paused
  ```
- `modules/aru/src/application.cppm` is the **partition** `aru:application`.
- `main.cpp` only needs to do `import aru;` and gets `Application` plus everything from `aru.standard`.
- The executable links the `aru_std` static library: `target_link_libraries(aru PRIVATE aru_std)`.

The facade/partition split is useful as the module grows: consumers keep importing a single name (`aru`), while the implementation is split into multiple partitions.

### Module `aru.standard`

- The static library target `aru_std` registers all `.cppm` files through `FILE_SET CXX_MODULES`.
- `modules/aru_std/src/lib.cppm` is the facade:
  ```cpp
  export module aru.standard;
  export import :print;
  export import :result;
  export import :string;
  export import :vector;
  ```
- `modules/aru_std/src/string.cppm` is the partition `aru.standard:string`. It exports the `character` concept and the `String<T>` class template.
- `modules/aru_std/src/result.cppm` is the partition `aru.standard:result`. It exports `Result<T, E>`.
- `modules/aru_std/src/print.cppm` is the partition `aru.standard:print`. It exports `println` and `eprintln`.
- `modules/aru_std/src/vector.cppm` is the partition `aru.standard:vector`. It exports `Vector<T>` with `VectorIterator` and `VectorIntoIter`.

#### Allocation rules (`String` and `Vector`)

Both types follow the same policy — initialization happens through factory functions, copy/move constructors are deleted, and copy/move happen through assignment or explicit member functions:

- `String<T>`: allocation via `String<T>::from(...)` (pointer or another string, cross-type converts and copies). Moving out via `s.move()` (same encoding only).
- `Vector<T>`: allocation via `Vector<T>::create(capacity)`, `create({...})`, or `create(container)` for any forward range with convertible elements (`std::array`, `std::vector`, `std::list`, ...), plus literal construction `Vector<int> t = {1, 2, 3}`. Copy via assignment `=` or `v.copy()`; move via move assignment or `v.move()`; literal assignment via `v = {1, 2, 3}`.
- Default constructors are private; copy/move constructors are deleted; destructors stay public (stack objects must be destructible).

#### `String` design

- `String<T>` is constrained by the exported `character` concept (`char8_t`, `char16_t`, `char32_t`).
- Storage is a `std::vector<T>` that always ends with a null terminator, so `stringData()` can hand out a C-string of the element type.
- `charSize` counts code units (excluding the terminator); `size` counts bytes (`charSize * sizeof(T)`).
- Conversion between encodings is **not** a mutating `encode()` method. A new string is created with the static `String<TargetT>::from(const String<SourceT>&)` factory, which decodes each code point from the source and re-encodes it into the target (see the transcoders in `aru::standard::detail`). Moving within the same encoding is done with `s.move()`.
- Invalid sequences decode to the replacement character `U+FFFD` (no exceptions).
- Different instantiations of `String<T>` are unrelated classes; the mutual `template<character U> friend class String;` declaration lets `from()` read the private data of `String<SourceT>`.
- `from(const String<SourceT>&)` with `SourceT == T` copies the storage directly (terminator included, no extra push).

### Module `aru.window` (paused)

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

- The public API takes UTF-8 text. With Clang, the source and execution character sets are UTF-8.
- `aru::standard::String<T>` stores text as `char8_t` (UTF-8), `char16_t` (UTF-16), or `char32_t` (UTF-32); see [the `String` reference](api/string.md). Conversion between the three encodings goes through the `String::from()` factory.
- Win32 `...W` APIs take UTF-16. The internal `toWide()` helper in `aru.window` converts UTF-8 titles with `MultiByteToWideChar(CP_UTF8, ...)` before calling `CreateWindowExW`. (Currently not compiled while `aru.window` is disabled.)
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
  │    ├─ (debug) println log
  │    └─ return std::expected<Application, ApplicationError>
  │         ├─ printInfo()
  │         └─ window::Window::create_window(title, width, height)
  │              ├─ toWide(title)  (UTF-8 → UTF-16)
  │              ├─ CreateWindowExW + ShowWindow
  │              └─ while (window.event_loop()) { ... }
  │                   └─ WM_DESTROY → PostQuitMessage → WM_QUIT → return false
  └─ current main.cpp: String::from(u8"Hello") → String<char32_t>::from(string)
```

There is no `Application::run()` in this iteration of the code yet. `main.cpp` currently only exercises the `String` API; the `Application`/window flow is kept ready for when `aru.window` is re-enabled.

## Error Handling

- The public API uses `std::expected`.
- No exceptions are thrown by engine code yet.
- `Application` uses `ApplicationError` (currently only `Unknown`).
- `aru.standard:result` provides `Result<T, E>` as a lightweight alternative for functions that always need to return one of two values; see [the `Result` reference](api/result.md).
- Future recommendation: grow the error enums/structs so they provide context.

## Logging

- Logging uses `std::println` and is only active in debug builds with `#ifndef NDEBUG`.
- The prefix currently used is: `Aru: Application(...)` and `Aru: Window(created!!), title: ...`.
- Target convention: `Aru: <Area>(<event>)`, for example `Aru: Window(created!!)`.
- Currently `init()` prints two similar log lines; this can be tidied up later.

## Build and Module Boundaries

- The root `CMakeLists.txt` adds `modules/aru` and `modules/aru_std`; `modules/aru_window` is currently commented out.
- Each module registers its `.cppm` files through `FILE_SET CXX_MODULES`.
- Standard headers are included in the **global module fragment** (`module;` ... `export module ...;`) so they do not leak to consumers.
- `aru` depends on `aru_std` through `target_link_libraries(aru PRIVATE aru_std)`.
- The `aru` facade re-exports `aru.standard`, so `import aru;` provides `String`, `Result`, `println`, and `Application`.

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
| `aru.window` is disabled in the build | Windowing is unavailable; the WIP `Window` refactor has a broken copy `operator=` | Finish the refactor, fix `operator=`, then re-enable `add_subdirectory` and `export import aru.window` |
| Window teardown is implicit | `DestroyWindow` is not called; cleanup relies on process exit | Add RAII teardown in `Window_T` or `Window` |
| No input handling | Keyboard and mouse messages are ignored | Handle `WM_KEYDOWN` / `WM_MOUSEMOVE` and expose events |
| No resize handling | `WM_SIZE` is not processed | Handle resize before the RHI needs it |
| `ApplicationError` is minimal | Only `Unknown`; no failure context | Expand the error enum/struct as failure paths appear |
| `String` is read-mostly | No `append`/`+`/`+=`, no `std::formatter`/`std::hash` support | Add concatenation, formatting, and hashing |
| `Vector` is basic | No `insert`/`erase`/`clear`, no bounds checking, no `std::formatter`/`std::hash` | Add the missing operations when needed |
| `Result` has no accessors | No `value()`/`error()`; only `value_or_unexpected()` | Add checked accessors |
| No tests yet | Regressions go undetected | Fill the `test/` folder with unit tests |
| Duplicate logs in `init()` | Repeated debug output | Clean up during refactor |
