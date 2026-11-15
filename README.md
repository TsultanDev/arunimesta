<div align="center">

# Arunimesta Engine

<img src="https://img.shields.io/badge/C%2B%2B-23-blue.svg" alt="C++23">
<img src="https://img.shields.io/badge/CMake-3.28%2B-064F8C.svg" alt="CMake 3.28+">
<img src="https://img.shields.io/badge/Clang-LLVM-black.svg" alt="Clang">
<img src="https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg" alt="Windows x64">
<img src="https://img.shields.io/badge/Status-Experimental-orange.svg" alt="Experimental">

</div>

**Arunimesta** is a 3D game engine built from scratch with **C++23** and *named modules*. The project emphasizes a modular architecture, explicit error handling, and RAII-based resource management.

> **Status: early development.** The build system and application module foundation are working. The windowing module is still a skeleton, and the Vulkan renderer has not been implemented.

---

## Project Status

| Area | Status |
| :--- | :--- |
| Build system (CMake + Clang + Ninja) | ✅ Working |
| Module `aru` (facade + `Application`) | ✅ Working |
| Module `aru.window` | 🚧 Skeleton / incomplete |
| Vulkan renderer (RHI) | ⬜ Not started |

The only thing that can currently be built and run is the `aru` executable, which creates an `Application` object, prints application info, and then exits.

---

## Current Features

- **C++23 Named Modules**: the main module `aru` with the module partition `aru:application` and the facade `lib.cppm`.
- **Factory + `std::expected`**: `Application::init()` returns `std::expected<Application, bool>` without throwing exceptions.
- **Pimpl with `std::unique_ptr`**: implementation details are hidden in `Application_T`; the object is *move-only*.
- **Heap allocation blocked**: `operator new` is deleted so objects can only be created through the factory and cannot be freely allocated on the heap.
- **RAII**: `Application_T` is cleaned up automatically by `std::unique_ptr`.
- **Debug logging**: `std::println` output is only active in debug builds (`#ifndef NDEBUG`).
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
| **Windowing** | `aru.window` — *work in progress* (Win32 target) |
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

Expected output (debug build):

```text
Aru: Application(created!!)
Aru: Application(created!!), name: Hello, version: 1
Aru: Application(created!!), name: Hello, version: 1
```

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
│       └── application.md           # Application API reference
│
├── modules/                         # All build targets
│   ├── aru/                         # Executable — entry point
│   │   ├── CMakeLists.txt
│   │   └── src/
│   │       ├── lib.cppm             # export module aru; (facade)
│   │       ├── application.cppm     # export module aru:application;
│   │       └── main.cpp             # int main()
│   │
│   └── aru_window/                  # Static library — windowing (WIP)
│       ├── CMakeLists.txt
│       └── src/
│           ├── lib.cppm             # facade (not filled in yet)
│           └── window.cppm          # aru.window:window skeleton
│
├── assets/                          # Assets (currently empty)
├── test/                            # Test area (currently empty)
└── build/                           # Build output (ignored)
```

### Module Descriptions

| Module | Type | Status | Description |
| :--- | :--- | :--- | :--- |
| `aru` | Executable | ✅ | Entry point; uses module `aru` and runs `Application`. |
| `aru:application` | Module partition | ✅ | `Application` implementation together with the internal `Application_T` details. |
| `aru.window` | Static library | 🚧 | Planned windowing abstraction; the source is still a skeleton. |

> Note: `modules/aru_window` has not been added to `add_subdirectory()` in the root `CMakeLists.txt`, so it is not built yet.

---

## Usage Example

The code currently in `modules/aru/src/main.cpp`:

```cpp
#include <expected>
#include <utility>
import aru;

int main(int argc, char** argv) {
    auto result = aru::Application::init("Hello", 1)
        .transform([](aru::Application&& app) -> void {
            app.printInfo();
        });

    return 0;
}
```

More explicit error handling:

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

Full reference: [`docs/api/application.md`](docs/api/application.md).

---

## Documentation

| Document | Contents |
| :--- | :--- |
| [docs/architecture.md](docs/architecture.md) | Module structure, pimpl, ownership, initialization flow |
| [docs/building.md](docs/building.md) | Prerequisites, presets, manual build, troubleshooting |
| [docs/coding-style.md](docs/coding-style.md) | Naming, `.cppm` file layout, factory/pimpl patterns |
| [docs/api/application.md](docs/api/application.md) | `aru::Application` API |
| [docs/roadmap.md](docs/roadmap.md) | Milestones and future plans |

---

## Roadmap Summary

- [x] CMake + Clang + Ninja build system
- [x] C++23 named modules and module partitions
- [x] `Application` with a `std::expected` factory and `unique_ptr` pimpl
- [ ] Finish the `aru.window` module and add it to the root build
- [ ] Vulkan RHI: instance, surface, adapter/device, swapchain
- [ ] Render pass, pipeline, command buffer, and render loop
- [ ] Event handling and resize
- [ ] Resource management and ECS

Details: [`docs/roadmap.md`](docs/roadmap.md).

---

## Notes

- This project is **under active development**; the API and structure may change without backward compatibility.
- The current build presets are specific to **Windows x64 + Clang**.
- The `aru_window` module is still incomplete and is not built yet.
- No license has been chosen yet.

---

## References

- [C++23 Modules — cppreference](https://en.cppreference.com/w/cpp/language/modules.html)
- [std::expected — cppreference](https://en.cppreference.com/w/cpp/utility/expected)
- [CMake: C++ Modules](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html)
- [Clang C++ Status](https://clang.llvm.org/cxx_status.html)
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
