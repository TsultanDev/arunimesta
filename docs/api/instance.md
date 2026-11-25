# `renaru::Instance` API Reference

`Instance` is the Vulkan instance wrapper in the `renaru:instance` module partition (`renaru/src/instance.cppm`). It is created through a static factory and returned in a `std::expected<Instance, Error>`.

The module uses the **Vulkan C API with manual dynamic dispatch**. `renaru::Entry` loads `vulkan-1.dll` and keeps `vkGetInstanceProcAddr` / `vkGetDeviceProcAddr` as fields; other commands are resolved on demand into local `PFN_vkXxx pfnXxx` variables. No `vulkan-1.lib` link is required.

## Header / Module

```cpp
import renaru;      // facade, re-exports renaru:instance
#include <expected>
#include <string>
#include <string_view>
```

For additional `renaru` translation units that need Vulkan types, include `<vulkan/vulkan.h>` directly. `VK_USE_PLATFORM_WIN32_KHR=1` is supplied by the `renaru` target's public compile definition, so Win32 surface types are available without a wrapper header. Dispatch state lives in the `renaru:entry` partition (`src/entry.cppm`) and is reached through `entry()`.

## API

### `static std::expected<Instance, Error> create(std::string_view sAppName)`

Creates the Vulkan instance.

- `sAppName` is a UTF-8 `std::string_view`; it does not need to be null-terminated.
- The application name is stored in the resulting `Instance` as `std::string`.
- The API version is **not** taken from the caller. `vkEnumerateInstanceVersion` is called and its result is used as `VkApplicationInfo::apiVersion`. If the loader does not expose that function (Vulkan 1.0), `VK_API_VERSION_1_0` is used.
- Global commands (`vkEnumerateInstanceVersion`, `vkCreateInstance`) are loaded from the `HMODULE` held by `Entry`; after creation, instance commands use `entry().resolveInstance(instance, ...)` and device commands use `entry().resolveDevice(device, ...)`.
- `VK_KHR_surface` and `VK_KHR_win32_surface` are enabled so that `createSurface()` works.

```cpp
auto instance = renaru::Instance::create("Arunimesta");
if (!instance) {
    std::println("error: {}", static_cast<std::uint32_t>(instance.error()));
    std::terminate();
}
```

### `Error`

| Value | Meaning |
| :--- | :--- |
| `eNone` | No error. |
| `eDynamicLoaderFailed` | `vkGetInstanceProcAddr` could not be loaded. |
| `eApiVersionQueryFailed` | `vkEnumerateInstanceVersion` returned a failure code. |
| `eInstanceCreationFailed` | `vkCreateInstance` failed. |
| `eSurfaceCreationFailed` | `vkCreateWin32SurfaceKHR` failed or the window was empty. |
| `eAdapterNotFound` | No physical device has a graphics + present queue family for the surface. |
| `eDeviceCreationFailed` | `vkCreateDevice` failed or the adapter was empty. |
| `eQueueRequestFailed` | The requested queue flags are unavailable on the selected family. |

`Error` lives in the `renaru:error` partition (`src/error.cppm`). There is no `std::formatter<Error, char>` specialization yet: print the numeric value with `std::println("{}", static_cast<std::uint32_t>(err))` until C++26 static reflection is available in Clang. The `renaru` facade re-exports that partition.

Failures are returned as `std::unexpected(Error::eX)`.

### Factory members returning `std::expected<T, Error>`

| Function | Defined in | Description |
| :--- | :--- | :--- |
| `createSurface(const winaru::Window&)` | `:surface` | Creates a Win32 Vulkan surface. |
| `requestAdapter(const Surface&)` | `:adapter` | Selects a physical device with graphics + present support. |

### Accessors

| Function | Description |
| :--- | :--- |
| `VkInstance handle() const noexcept` | The underlying Vulkan instance handle. |
| `std::uint32_t apiVersion() const noexcept` | The version returned by `vkEnumerateInstanceVersion`. |
| `const std::string& applicationName() const noexcept` | Copy of the application name. |
| `bool empty() const noexcept` | `true` when no instance handle is held. |

### Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; `VkInstance` is a raw handle. |
| Move constructor / move assignment | Steals the `VkInstance` handle and the string, then empties the source. `noexcept`. |
| Destructor | Calls `vkDestroyInstance` only when a handle is held. |

Only one object owns a given Vulkan handle; use `std::move` to transfer ownership or store `Instance` in a container.

## Build Notes

- `renaru/CMakeLists.txt` registers all partitions in a `FILE_SET CXX_MODULES`, links `Vulkan::Headers` (C headers only) publicly, and adds `VK_USE_PLATFORM_WIN32_KHR=1` as a public compile definition.
- The module is registered in the root `CMakeLists.txt`.

## Limitations

- No debug utils messenger yet.
- No validation-layer configuration.
