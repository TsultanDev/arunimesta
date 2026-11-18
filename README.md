<div align="center">

# Arunimesta Engine

<img src="https://img.shields.io/badge/C%2B%2B-23-blue.svg" alt="C++23">
<img src="https://img.shields.io/badge/CMake-3.28%2B-064F8C.svg" alt="CMake 3.28+">
<img src="https://img.shields.io/badge/Clang-LLVM-black.svg" alt="Clang">
<img src="https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg" alt="Windows x64">
<img src="https://img.shields.io/badge/Status-Experimental-orange.svg" alt="Experimental">

</div>

**Arunimesta** is a 3D game engine built from scratch with **C++23** and *named modules*. The project emphasizes a modular architecture, explicit error handling, and RAII-based resource management.

> **Status: early development.** The build system, the application module, and the standard library module (`aru.standard` with `String`, `Result`, and `println`) are working. The `aru.window` module is being refactored and is temporarily disabled in the build. The Vulkan renderer has not been implemented.

---

## Project Status

| Area | Status |
| :--- | :--- |
| Build system (CMake + Clang + Ninja) | ✅ Working |
| Module `aru` (facade + `Application`) | ✅ Working |
| Module `aru.standard` (`String`, `Result`, `println`) | ✅ Working |
| Module `aru.window` | ⏸ Paused (refactoring; disabled in build) |
| Vulkan renderer (RHI) | ⬜ Not started |

The `aru` executable currently demonstrates `aru::standard::String`: it creates a UTF-8 string and converts it to a new UTF-32 string through the `String::from()` factory.

---

## Current Features

- **C++23 Named Modules**: the main module `aru` with the module partition `aru:application` and the facade `lib.cppm`.
- **UTF String Type**: `aru::standard::String<T>` supports `char8_t`, `char16_t`, and `char32_t`. A new string of any target type is created from an existing string with the static `String::from()` factory, which transcodes between UTF-8, UTF-16, and UTF-32. Invalid sequences are replaced with `U+FFFD`. Moving within the same encoding is done with `s.move()`.
- **Vector Type**: `aru::standard::Vector<T>` is a growable dynamic array. Allocation is factory-only (`Vector<T>::create()`) or through a literal list (`Vector<int> t = {1, 2, 3};`), copying goes through assignment or `copy()`, moving through move assignment or `move()`, and literal assignment through `v = {1, 2, 3}`. Copy/move constructors are deleted.
- **Result Type**: `aru::standard::Result<T, E>` is a lightweight discriminated union for explicit success/failure values without exceptions.
- **Factory + `std::expected`**: `Application::init()` returns `std::expected<Application, ApplicationError>` without throwing exceptions.
- **Pimpl with `std::unique_ptr`**: implementation details are hidden in `Application_T`; the object is *move-only*.
- **Heap allocation blocked**: `operator new` is deleted so objects can only be created through the factory and cannot be freely allocated on the heap.
- **RAII**: `Application_T` is cleaned up automatically by `std::unique_ptr`.
- **Debug logging**: `std::println` output is only active in debug builds (`#ifndef NDEBUG`).
- **Win32 windowing**: `aru.window` creates and shows a window, runs a polling event loop, and exits the loop when the window is destroyed.
- **Unicode window titles**: titles are passed as UTF-8 `std::string` and converted to UTF-16 with `MultiByteToWideChar`, so non-ASCII titles render correctly.
- **CMake Presets**: Clang LLVM x64 presets for Debug and Release, complete with `compile_commands.json` for clangd.
- **Modular structure**: each module lives in `modules/<module_name>` with its own `CMakeLists.txt`.

---

## Tech Stack

| Component | Technology |
| :--- | :--- |
| **Language** | C++23 (named modules) |
| **Compiler** | Clang (`clang++`) |
| **Build System** | CMake ≥ 3.28 + Ninja |
| **Standard Features** | `std::expected`, `std::print` / `std::println`, `std::unique_ptr` |
| **Target Platform** | Windows x64 (matching the available presets) |
| **Standard Library** | `aru.standard` — `String` (UTF-8/16/32), `Vector`, `Result`, debug `println`/`eprintln` |
| **Windowing** | `aru.window` — Win32 Unicode APIs, window + event loop (*paused*) |
| **Rendering** | Vulkan — *planned* |

---

## Prerequisites

1. **Clang** with C++23 modules support; `clang++` must be available in `PATH`.
2. **CMake ≥ 3.28** — required for `FILE_SET ... TYPE CXX_MODULES`.
3. **Ninja** as the generator.
4. **Windows x64** — the provided presets are only active on Windows.

> The Vulkan SDK is not required until the RHI module starts being implemented.

---

## Build

### 1. Clone the Repository

```bash
git clone https://github.com/TsultanDev/arunimesta.git
cd arunimesta
```

### 2. Configure

Debug:

```bash
cmake --preset clang-x64-debug
```

Release:

```bash
cmake --preset clang-x64-release
```

### 3. Build

```bash
cmake --build --preset build-debug
```

or:

```bash
cmake --build --preset build-release
```

### 4. Run

```bash
./build/clang-x64-debug/modules/aru/aru.exe
```

The current demo program prints nothing; it only exercises `String` creation and conversion.

### Manual Build

If you prefer not to use the presets:

```bash
cmake -S . -B build/clang-x64-debug -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build/clang-x64-debug
```

Details and troubleshooting are in [`docs/building.md`](docs/building.md).

---

## Project Structure

```text
arunimesta/
├── .clangd                          # clangd configuration (compile_commands.json)
├── .gitignore
├── CMakeLists.txt                   # Root build script
├── CMakePresets.json                # Clang/Ninja presets (Debug & Release)
├── README.md
│
├── docs/                            # Project documentation
│   ├── README.md                    # Documentation index
│   ├── architecture.md              # Architecture & module design
│   ├── building.md                  # Build guide & troubleshooting
│   ├── coding-style.md              # Code conventions
│   ├── roadmap.md                   # Development plan
│   └── api/
│       ├── application.md           # Application API reference
│       ├── string.md                # String API reference
│       ├── vector.md                # Vector API reference
│       ├── result.md                # Result API reference
│       └── print.md                 # print API reference
│
├── modules/                         # All build targets
│   ├── aru/                         # Executable — entry point
│   │   ├── CMakeLists.txt
│   │   └── src/
│   │       ├── lib.cppm             # export module aru; (facade)
│   │       ├── application.cppm     # export module aru:application;
│   │       └── main.cpp             # int main()
│   │
│   ├── aru_std/                     # Static library — standard library module
│   │   ├── CMakeLists.txt
│   │   └── src/
│   │       ├── lib.cppm             # export module aru.standard; (facade)
│   │       ├── string.cppm          # String<T> (UTF-8/16/32) + transcoding
│   │       ├── vector.cppm          # Vector<T> + iter() / intoIter()
│   │       ├── result.cppm          # Result<T, E> discriminated union
│   │       └── print.cppm           # println / eprintln (debug builds)
│   │
│   └── aru_window/                  # Static library — windowing (Win32, paused)
│       ├── CMakeLists.txt
│       └── src/
│           ├── lib.cppm             # export module aru.window; (facade)
│           └── window.cppm          # Win32 window + event loop
│
├── assets/                          # Assets (currently empty)
├── test/                            # Test area (currently empty)
└── build/                           # Build output (ignored)
```

### Module Descriptions

| Module | Type | Status | Description |
| :--- | :--- | :--- | :--- |
| `aru` | Executable | ✅ | Entry point; imports module `aru`, which re-exports `aru.standard`. |
| `aru:application` | Module partition | ✅ | `Application` implementation together with the internal `Application_T` details. |
| `aru.standard` | Static library | ✅ | Facade `lib.cppm` over the `:string`, `:vector`, `:result`, and `:print` partitions. |
| `aru.standard:string` | Module partition | ✅ | `String<T>` for `char8_t`/`char16_t`/`char32_t` with transcoding `from()` and `move()`. |
| `aru.standard:vector` | Module partition | ✅ | `Vector<T>` with factory-only allocation (`create()`), `copy()`/`move()`, `iter()`/`intoIter()`. |
| `aru.standard:result` | Module partition | ✅ | `Result<T, E>` discriminated union. |
| `aru.standard:print` | Module partition | ✅ | Debug-gated `println` / `eprintln` wrappers. |
| `aru.window` | Static library | ⏸ | Win32 window creation and event loop; temporarily disabled in the build. |

> `modules/aru` links the `aru_std` static library (`target_link_libraries(aru PRIVATE aru_std)`). `modules/aru_window` is registered in the root `CMakeLists.txt` but is currently commented out while it is being refactored.

---

## Usage Example

The code currently in `modules/aru/src/main.cpp`:

```cpp
#include <expected>
#include <print>
import aru;

int main() {
    auto string = aru::standard::String<char8_t>::from(u8"Hello");
    auto s2 = aru::standard::String<char32_t>::from(string);
    return 0;
}
```

Strings are not converted in place. A **new** string is created from an existing one with the static `from()` factory; the target type is the `String` type the factory is called on:

```cpp
import aru;

auto u8s  = aru::standard::String<char8_t>::from(u8"halo 🌍 dunia");
auto u16s = aru::standard::String<char16_t>::from(u8s);  // UTF-8 -> UTF-16
auto u32s = aru::standard::String<char32_t>::from(u16s); // UTF-16 -> UTF-32
auto u8b  = aru::standard::String<char8_t>::from(u32s);  // UTF-32 -> UTF-8
// u8b == u8s: roundtrip is lossless
```

Same-encoding moves use `move()`; the source becomes empty:

```cpp
auto s8b = u8s.move();   // UTF-8 -> UTF-8, same type only
```

Vectors are allocated through the `create()` factory, filled by assignment, and copied/moved via assignment or `copy()`/`move()`:

```cpp
auto vec = aru::standard::Vector<int>::create(16);
vec = {1, 3, 4};
auto copy = vec.copy();
auto moved = vec.move();   // vec is now empty
```

Literal construction works too: `Vector<int> t = {1, 2, 3};` (requires a copy-constructible element type).

Application with explicit error handling:

```cpp
import aru;

int main() {
    auto app = aru::Application::init("Arunimesta", 1);
    if (!app) {
        // init() failed
        return 1;
    }

    app->printInfo();
    return 0;
}
```

Window creation with explicit error handling (`aru.window` is currently disabled in the build):

```cpp
auto window = aru::window::Window::create_window("Arunimesta — 世界", 800, 600);
if (!window) {
    return 1;
}

while (window->event_loop()) {
}
```

Titles are UTF-8, so non-ASCII text such as `世界` is supported.

References: [`docs/api/application.md`](docs/api/application.md), [`docs/api/string.md`](docs/api/string.md), [`docs/api/vector.md`](docs/api/vector.md), [`docs/api/result.md`](docs/api/result.md), and [`docs/api/print.md`](docs/api/print.md).

---

## Documentation

| Document | Contents |
| :--- | :--- |
| [docs/architecture.md](docs/architecture.md) | Module structure, pimpl, ownership, initialization flow |
| [docs/building.md](docs/building.md) | Prerequisites, presets, manual build, troubleshooting |
| [docs/coding-style.md](docs/coding-style.md) | Naming, `.cppm` file layout, factory/pimpl patterns |
| [docs/api/application.md](docs/api/application.md) | `aru::Application` API |
| [docs/api/string.md](docs/api/string.md) | `aru::standard::String` API (UTF-8/16/32) |
| [docs/api/vector.md](docs/api/vector.md) | `aru::standard::Vector` API (factory-only allocation) |
| [docs/api/result.md](docs/api/result.md) | `aru::standard::Result` API |
| [docs/api/print.md](docs/api/print.md) | `aru::standard::println` / `eprintln` API |
| [docs/api/window.md](docs/api/window.md) | `aru::window::Window` API (paused) |
| [docs/roadmap.md](docs/roadmap.md) | Milestones and future plans |

---

## Roadmap Summary

- [x] CMake + Clang + Ninja build system
- [x] C++23 named modules and module partitions
- [x] `Application` with a `std::expected` factory and `unique_ptr` pimpl
- [x] `aru.standard`: `String` (UTF-8/16/32 with transcoding), `Vector`, `Result`, debug `println`
- [ ] Finish and re-enable `aru.window`: RAII window creation and polling event loop
- [ ] Vulkan RHI: instance, surface, adapter/device, swapchain
- [ ] Render pass, pipeline, command buffer, and render loop
- [ ] Event handling and resize
- [ ] Resource management and ECS

Details: [`docs/roadmap.md`](docs/roadmap.md).

---

## Notes

- This project is **under active development**; the API and structure may change without backward compatibility.
- The current build presets are specific to **Windows x64 + Clang**.
- The `aru.window` module is **temporarily disabled in the build** while it is refactored; the API shown in [its reference](docs/api/window.md) may change.
- Window titles are UTF-8 and may contain non-ASCII characters.
- No license has been chosen yet.

---

## References

- [C++23 Modules — cppreference](https://en.cppreference.com/w/cpp/language/modules.html)
- [std::expected — cppreference](https://en.cppreference.com/w/cpp/utility/expected)
- [CMake: C++ Modules](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html)
- [Clang C++ Status](https://clang.llvm.org/cxx_status.html)
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
