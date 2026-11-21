<div align="center">

<img src="assets/logo.png" alt="Arunimesta logo" width="160">

# Arunimesta Engine

<img src="https://img.shields.io/badge/C%2B%2B-23-blue.svg" alt="C++23">
<img src="https://img.shields.io/badge/CMake-3.28%2B-064F8C.svg" alt="CMake 3.28+">
<img src="https://img.shields.io/badge/Clang-LLVM-black.svg" alt="Clang">
<img src="https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg" alt="Windows x64">
<img src="https://img.shields.io/badge/Status-Experimental-orange.svg" alt="Experimental">

</div>

**Arunimesta** is a 3D game engine built from scratch with **C++23** and *named modules*. The project emphasizes a modular architecture, explicit error handling, and RAII-based resource management.

> **Status: early development.** The build system, the standard library module (`aru.standard` with `Array`, `String`, `Vector`, `Result`, and `println`), the Win32 windowing module (`aru.window`), and the Vulkan RHI presentation foundation (`aru.rhi` instance, surface, adapter, device, queue, swapchain, resource views, render pass, and framebuffers with dynamic dispatch) are working. Rendering and presentation of an actual frame have not been implemented yet.

## Preview

<p align="center">
  <img src="assets/window.png" alt="Arunimesta Win32 window" width="700">
</p>

<p align="center"><em>The current Win32 window created by <code>aru.window</code>. Rendering is not implemented yet.</em></p>

---

## Project Status

| Area | Status |
| :--- | :--- |
| Build system (CMake + Clang + Ninja) | ✅ Working |
| Module `aru` (facade + `Application`) | ✅ Working |
| Module `aru.standard` (`String`, `Result`, `println`) | ✅ Working |
| Module `aru.window` | ✅ Active (Win32 window + blocking event loop) |
| Vulkan RHI (`aru.rhi`) | ✅ Basic (instance, surface, adapter, device, queue, swapchain, resource, render pass, framebuffer) |

The `aru` executable creates a Win32 window, a Vulkan instance, surface, adapter, device, graphics/present queue, swapchain with image views, a render pass, and one framebuffer per swapchain image before entering the window event loop.

---

## Current Features

- **C++23 Named Modules**: the main module `aru` with the module partition `aru:application` and the facade `lib.cppm`.
- **UTF String Type**: `aru::standard::String<T>` supports `char8_t`, `char16_t`, and `char32_t`. Use the static `String::from()` factory to build one from a C string of any character type (`char` is treated as UTF-8) or from another `String<T>`; it transcodes between UTF-8, UTF-16, and UTF-32. Invalid sequences are replaced with `U+FFFD`. Any `String<T>` can be printed directly with `println()` thanks to its `std::formatter` specialization. Moving within the same encoding is done with `s.move()`.
- **Array Type**: `aru::standard::Array<T, N>` is a fixed-size array with inline `T data[N]` storage (no heap allocation). Create with `Array<T, N>::create()` or `create({...})`; copy/move through constructors, assignment, or `copy()`/`move()`. Rule of Five is defaulted because there is no owned resource to steal.
- **Vector Type**: `aru::standard::Vector<T>` is a growable dynamic array. Allocation goes through `Vector<T>::create(capacity)`, `create({...})`, or `create(container)` for any `std::array`/`std::vector`/forward range with convertible elements. Literal construction (`Vector<int> t = {1, 2, 3};`) is also supported. Copy/move constructors, assignment, `copy()`, and `move()` are all available (Rule of Five).
- **Result Type**: `aru::standard::Result<T, E>` is a lightweight discriminated union for explicit success/failure values without exceptions.
- **Option Type**: `aru::standard::Option<T>` holds either a value or nothing. Create with `Option<T>::some(value)` / `none()`, then use `map()`, `andThen()`, `orElse()`, and `valueOr()`; copy/move follow the engine's Rule of Five convention.
- **Factory + `std::expected`**: `Application::init()` returns `std::expected<Application, ApplicationError>` without throwing exceptions.
- **Pimpl with `std::unique_ptr`**: implementation details are hidden in `Application_T`; the object is *move-only*.
- **Heap allocation blocked**: `operator new` is deleted so objects can only be created through the factory and cannot be freely allocated on the heap.
- **RAII**: `Application_T` is cleaned up automatically by `std::unique_ptr`.
- **Debug logging**: `std::println` output is only active in debug builds (`#ifndef NDEBUG`).
- **Win32 windowing**: `aru.window` creates and shows a window via `Window::create()`, runs a blocking event loop with `event_loop()`, and exits when the window is destroyed.
- **Unicode window titles**: narrow UTF-8 titles are converted to UTF-16 with `String<char8_t>` → `String<char16_t>`; a `const char16_t*` overload is available too.
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
| **Windowing** | `aru.window` — Win32 Unicode APIs, window + blocking event loop |
| **Rendering** | Vulkan-Hpp (dynamic dispatch) — `aru.rhi` instance, surface, adapter, device, queue, swapchain, resource, render pass, framebuffer |

---

## Prerequisites

1. **Clang** with C++23 modules support; `clang++` must be available in `PATH`.
2. **CMake ≥ 3.28** — required for `FILE_SET ... TYPE CXX_MODULES`.
3. **Ninja** as the generator.
4. **Windows x64** — the provided presets are only active on Windows.

> The Vulkan SDK is required by `aru_rhi` for the Vulkan-Hpp headers. Dynamic dispatch is used, so `vulkan-1.lib` is not linked.

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

The current demo creates a Win32 window, a Vulkan instance, surface, adapter, device, graphics/present queue, swapchain, image views, a render pass, and framebuffers, then enters the window event loop.

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
│       ├── option.md                # Option<T> API reference
│       ├── string.md                # String API reference
│       ├── vector.md                # Vector API reference
│       ├── result.md                # Result API reference
│       ├── print.md                 # print API reference
│       ├── window.md                # Window API reference
│       ├── swapchain.md             # Swapchain API reference
│       ├── resource.md              # Resource tagged union reference
│       ├── render_pass.md           # RenderPass API reference
│       └── framebuffer.md           # Framebuffer API reference
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
│   │       ├── array.cppm           # Array<T, N> (inline storage)
│   │       ├── option.cppm          # Option<T> (some/none + monadic helpers)
│   │       ├── string.cppm          # String<T> (UTF-8/16/32) + transcoding
│   │       ├── vector.cppm          # Vector<T> + iter() / intoIter()
│   │       ├── result.cppm          # Result<T, E> discriminated union
│   │       └── print.cppm           # println / eprintln (debug builds)
│   │
│   └── aru_window/                  # Static library — windowing (Win32)
│       ├── CMakeLists.txt
│       └── src/
│           ├── lib.cppm             # export module aru.window; (facade)
│           ├── error.cppm           # aru.window:error (formattable Error)
│           └── window.cppm          # Win32 window + event loop
│   │
│   └── aru_rhi/                     # Static library — Vulkan RHI
│       ├── CMakeLists.txt
│       ├── include/aru_rhi/vulkan.hpp # Shared Vulkan-Hpp + dynamic dispatch
│       └── src/
│           ├── lib.cppm             # export module aru.rhi; (facade)
│           ├── error.cppm           # aru.rhi:error (formattable Error)
│           ├── instance.cppm        # Instance + QueueFlags
│           ├── surface.cppm         # Win32 Surface
│           ├── adapter.cppm         # Physical device selection
│           ├── device.cppm          # Logical device + factories
│           ├── queue.cppm           # Queue + requestQueue()
│           ├── resource.cppm        # Resource tagged union (image/buffer)
│           ├── swapchain.cppm       # Swapchain + image views
│           ├── render_pass.cppm     # RenderPass
│           └── framebuffer.cppm     # Framebuffer
│
├── assets/                          # Logo and documentation images
│   ├── logo.png                     # Project logo (README header)
│   └── window.png                   # Win32 window screenshot (README, docs)
├── test/                            # Test area (currently empty)
└── build/                           # Build output (ignored)
```

### Module Descriptions

| Module | Type | Status | Description |
| :--- | :--- | :--- | :--- |
| `aru` | Executable | ✅ | Entry point; imports module `aru`, which re-exports `aru.standard`. |
| `aru:application` | Module partition | ✅ | `Application` implementation together with the internal `Application_T` details. |
| `aru.standard` | Static library | ✅ | Facade `lib.cppm` over the `:array`, `:string`, `:vector`, `:result`, and `:print` partitions. |
| `aru.standard:string` | Module partition | ✅ | `String<T>` for `char8_t`/`char16_t`/`char32_t` with transcoding `from()` (any char pointer or `String`), `move()`, and `std::formatter`. |
| `aru.standard:array` | Module partition | ✅ | `Array<T, N>` fixed-size array with inline `T data[N]`, `copy()`/`move()` + assignment. |
| `aru.standard:vector` | Module partition | ✅ | `Vector<T>` with factory-only allocation (`create()`), `copy()`/`move()`, `iter()`/`intoIter()`. |
| `aru.standard:result` | Module partition | ✅ | `Result<T, E>` discriminated union. |
| `aru.standard:print` | Module partition | ✅ | Debug-gated `println` / `eprintln` wrappers. |
| `aru.window` | Static library | ✅ | Win32 window creation, `Result`-based errors, copy/move by assignment, `createFromWindow()`, and event loop. |
| `aru.rhi` | Static library | ✅ | Vulkan-Hpp with dynamic dispatch: instance, surface, adapter, device, queue, swapchain, resource, render pass, framebuffer. |

> `modules/aru` links `aru_std`, `aru_window`, and `aru_rhi`. The `aru` facade re-exports `aru.standard`, `aru.window`, and `aru.rhi`.

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
auto vec = aru::standard::Vector<int>::create(16);       // capacity
auto vals = aru::standard::Vector<int>::create({1, 3, 4}); // literal list

std::array<int, 3> src = {1, 3, 4};
auto fromArray = aru::standard::Vector<int>::create(src); // any forward range

vec = {1, 3, 4};
auto copy = vec.copy();
auto moved = vec.move();   // vec is now empty

// Three iteration styles:
for (auto value : vec)          { /* copy */ }
for (auto& value : vec.iter())  { /* reference */ }
for (auto value : vec.intoIter()) { /* move out; vec becomes empty */ }
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

Window creation with explicit error handling:

```cpp
auto window = aru::window::Window::create("Arunimesta — 世界", 800, 600)
    .valueOrUnexpected([](aru::window::Error&& err) {
        aru::standard::println("Error! : {}", err);
    });

window.event_loop();
```

Narrow titles are UTF-8, so non-ASCII text such as `世界` is supported. Rule of Five is implemented: copy duplicates the title/metadata (not the `HWND`), move transfers the handle. `window.copy()`, `window.move()`, and assignment are also available. To make a real second OS window, use `Window::createFromWindow(window)`.

Vulkan RHI setup (see [`docs/api/swapchain.md`](docs/api/swapchain.md), [`docs/api/resource.md`](docs/api/resource.md), [`docs/api/render_pass.md`](docs/api/render_pass.md), and [`docs/api/framebuffer.md`](docs/api/framebuffer.md) for the full flow):

```cpp
auto swapchain = device.createSwapchain(window.windowWidth(), window.windowHeight())
    .valueOrUnexpected(report);
auto images = device.createSwapchainImages(swapchain).valueOrUnexpected(report);
auto renderPass = device.createRenderPass(swapchain).valueOrUnexpected(report);

auto framebuffers = aru::standard::Vector<aru::rhi::Framebuffer>::create(images.length());
for (const auto& image : images) {
    auto framebuffer = device.createFramebuffer(renderPass, image).valueOrUnexpected(report);
    framebuffers.push(framebuffer.move()); // transfer ownership into the vector
}
```

References: [`docs/api/application.md`](docs/api/application.md), [`docs/api/option.md`](docs/api/option.md), [`docs/api/string.md`](docs/api/string.md), [`docs/api/vector.md`](docs/api/vector.md), [`docs/api/result.md`](docs/api/result.md), [`docs/api/print.md`](docs/api/print.md), [`docs/api/window.md`](docs/api/window.md), [`docs/api/swapchain.md`](docs/api/swapchain.md), [`docs/api/resource.md`](docs/api/resource.md), [`docs/api/render_pass.md`](docs/api/render_pass.md), and [`docs/api/framebuffer.md`](docs/api/framebuffer.md).

---

## Documentation

| Document | Contents |
| :--- | :--- |
| [docs/architecture.md](docs/architecture.md) | Module structure, pimpl, ownership, initialization flow |
| [docs/building.md](docs/building.md) | Prerequisites, presets, manual build, troubleshooting |
| [docs/coding-style.md](docs/coding-style.md) | Naming, `.cppm` file layout, factory/pimpl patterns |
| [docs/assets.md](docs/assets.md) | Logo, screenshots, and asset usage rules |
| [docs/api/application.md](docs/api/application.md) | `aru::Application` API |
| [docs/api/array.md](docs/api/array.md) | `aru::standard::Array` API (fixed-size) |
| [docs/api/option.md](docs/api/option.md) | `aru::standard::Option<T>` API (some/none, map/andThen, Rule of Five) |
| [docs/api/string.md](docs/api/string.md) | `aru::standard::String` API (UTF-8/16/32) |
| [docs/api/vector.md](docs/api/vector.md) | `aru::standard::Vector` API (factory-only allocation) |
| [docs/api/result.md](docs/api/result.md) | `aru::standard::Result` API |
| [docs/api/print.md](docs/api/print.md) | `aru::standard::println` / `eprintln` API |
| [docs/api/window.md](docs/api/window.md) | `aru::window::Window` API (Win32, active) |
| [docs/api/swapchain.md](docs/api/swapchain.md) | `aru::rhi::Swapchain` API and image selection |
| [docs/api/resource.md](docs/api/resource.md) | `aru::rhi::Resource` tagged union (image/buffer) |
| [docs/api/render_pass.md](docs/api/render_pass.md) | `aru::rhi::RenderPass` API |
| [docs/api/framebuffer.md](docs/api/framebuffer.md) | `aru::rhi::Framebuffer` API |
| [docs/roadmap.md](docs/roadmap.md) | Milestones and future plans |

---

## Roadmap Summary

- [x] CMake + Clang + Ninja build system
- [x] C++23 named modules and module partitions
- [x] `Application` with a `std::expected` factory and `unique_ptr` pimpl
- [x] `aru.standard`: `String` (UTF-8/16/32 with transcoding), `Vector`, `Result`, debug `println`
- [x] Re-enable and implement `aru.window`: RAII window creation, copy/move by assignment, and blocking event loop
- [x] Vulkan RHI: instance, surface, adapter/device, queue
- [x] Vulkan swapchain, image views, resource union, render pass, and framebuffers
- [ ] Present an actual frame (command buffers, pipeline, synchronization, render loop)
- [ ] Event handling and resize
- [ ] Resource management and ECS

Details: [`docs/roadmap.md`](docs/roadmap.md).

---

## Contributing

Contributions are welcome. Start with the [contributing guide](CONTRIBUTING.md), which covers the build, coding style, testing expectations, and pull request process.

- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security Policy](SECURITY.md) — report vulnerabilities privately, not as public issues
- [Issue templates](.github/ISSUE_TEMPLATE) — bug reports and feature requests
- [Documentation index](docs/README.md)

## License

Arunimesta is released under the [MIT License](LICENSE).

## Notes

- This project is **under active development**; the API and structure may change without backward compatibility.
- The current build presets are specific to **Windows x64 + Clang**.
- The `aru.window` module is active on Windows x64; see [its reference](docs/api/window.md) for the current API.
- Window titles are UTF-8 and may contain non-ASCII characters.

---

## References

- [C++23 Modules — cppreference](https://en.cppreference.com/w/cpp/language/modules.html)
- [std::expected — cppreference](https://en.cppreference.com/w/cpp/utility/expected)
- [CMake: C++ Modules](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html)
- [Clang C++ Status](https://clang.llvm.org/cxx_status.html)
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
