# Build Guide

This document explains how to build Arunimesta from source, both with presets and manually.

## Prerequisites

| Requirement | Version / Notes |
| :--- | :--- |
| Clang | Must support C++23 modules; `clang++` available in `PATH` |
| CMake | ≥ 3.28 (required for `FILE_SET ... TYPE CXX_MODULES`) |
| Ninja | The generator used by the presets |
| OS | Windows x64 — the available presets are only active on Windows |

You can check the installed tools:

```bash
clang++ --version
cmake --version
ninja --version
```

> The Vulkan SDK is required by `aru_rhi` for Vulkan-Hpp headers. Dynamic dispatch is used, so `vulkan-1.lib` is not linked.

## Available Presets

| Configure Preset | Build Preset | Build Type | Output Directory |
| :--- | :--- | :--- | :--- |
| `clang-x64-debug` | `build-debug` | Debug | `build/clang-x64-debug` |
| `clang-x64-release` | `build-release` | Release | `build/clang-x64-release` |

The presets use:

- The **Ninja** generator
- The **clang++** compiler
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` (for clangd)
- The `hostSystemName == Windows` condition

## Building with Presets

### Debug

```bash
cmake --preset clang-x64-debug
cmake --build --preset build-debug
```

### Release

```bash
cmake --preset clang-x64-release
cmake --build --preset build-release
```

## Running

Debug executable:

```bash
./build/clang-x64-debug/modules/aru/aru.exe
```

Release executable:

```bash
./build/clang-x64-release/modules/aru/aru.exe
```

## Manual Build

If the presets do not fit your environment:

```bash
cmake -S . -B build/manual -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build/manual
```

The result is located at `build/manual/modules/aru/aru.exe`.

## Clean Rebuild

If you encounter strange errors caused by the cache or old modules:

```bash
rm -rf build/clang-x64-debug      # Bash / Git Bash
```

```powershell
Remove-Item -Recurse -Force build\clang-x64-debug   # PowerShell
```

Then configure and build again.

## clangd Integration

- `CMakePresets.json` exports `compile_commands.json`.
- `.clangd` points clangd to `build/clang-x64-debug`.
- Therefore, configure the Debug preset first so that the IDE/clangd recognizes the modules and includes.

## C++ Standard Library

There is no separate standard-library module or target. Engine code uses the C++ standard library directly: `std::expected<T, E>` with `std::unexpected(...)` for fallible operations, `std::optional<T>`, `std::string` / `std::u8string` / `std::u16string` / `std::u32string` / `std::wstring`, `std::string_view`, `std::vector<T>`, `std::array<T, N>`, and `std::println` / `std::eprintln`. The `aru` facade re-exports only `aru.window` and `aru.rhi`.

## Module `aru_window` (active)

`modules/aru_window` contains the Win32 windowing module `aru.window`. It is registered in the root `CMakeLists.txt` and re-exported by the `aru` facade:

```cmake
# Root CMakeLists.txt
add_subdirectory("modules/aru_window")
add_subdirectory("modules/aru_rhi")
add_subdirectory("modules/aru")

# modules/aru/CMakeLists.txt
target_link_libraries(aru PRIVATE aru_window aru_rhi)
```

The target defines `UNICODE` and `_UNICODE`, and links `user32` and `gdi32`. Windows are created with `Window::create()`; narrow UTF-8 titles are converted to UTF-16 with `MultiByteToWideChar(CP_UTF8, ...)`, a direct `const char16_t*` overload is available, and `windowTitle()` returns `const std::wstring&`. See the [Window API](api/window.md).

## Troubleshooting

| Symptom | Likely Cause | Solution |
| :--- | :--- | :--- |
| `clang++: command not found` | Clang is not installed / not in `PATH` | Install LLVM and make sure `clang++` can be invoked |
| `CMake ... does not support CXX_MODULES` | CMake is too old | Use CMake ≥ 3.28 |
| Preset does not appear / condition error | Presets are only active on Windows | Use Windows, or create your own preset/manual configuration |
| `ninja: command not found` | Ninja is not installed | Install Ninja or use another generator |
| Error while compiling `.cppm` | The compiler does not support modules | Use a Clang version that supports C++23 modules |
| Header changes are not detected | Stale build/module cache | Delete the build directory and configure again |
| `import aru;` fails | The `.cppm` file is not listed in `FILE_SET CXX_MODULES`, or the target is not linked | Check `target_sources` and `target_link_libraries` |
| Window title shows garbled or CJK characters | ANSI (`...A`) and Unicode (`...W`) Win32 APIs are mixed | Use the `...W` variants consistently and convert UTF-8 titles to UTF-16 with `MultiByteToWideChar(CP_UTF8, ...)` (or use the `const char16_t*` overload) |

## Notes

- All module files must be listed in `target_sources(... FILE_SET CXX_MODULES ...)` to be compiled.
- External library includes go in the global module fragment, not after `export module`.
- Debug builds enable `std::println` logging; Release builds do not.
- The `aru_window` target defines `UNICODE` and `_UNICODE`; keep Win32 code on the Unicode (`...W`) APIs.
- `modules/aru_window` and `modules/aru_rhi` are active targets; new partitions must be added to their `FILE_SET CXX_MODULES` lists.
