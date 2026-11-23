# `aru::rhi::Instance` API Reference

`Instance` is the Vulkan instance wrapper in the `aru.rhi:instance` module partition (`aru_rhi/src/instance.cppm`). It is created through a static factory and returned in a `std::expected<Instance, Error>`.

The module uses **Vulkan-Hpp dynamic dispatch**: `vkGetInstanceProcAddr` and the instance entry points are resolved at runtime with `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC`. No `vulkan-1.lib` link is required.

## Header / Module

```cpp
import aru.rhi;      // facade, re-exports aru.rhi:instance
#include <expected>
#include <string>
#include <string_view>
```

For additional `aru_rhi` translation units that need Vulkan types, include `aru_rhi/vulkan.hpp`. That header sets up dynamic dispatch and declares the dispatcher as `extern`. The single definition (main storage) lives in `src/instance.cppm`.

## API

### `static std::expected<Instance, Error> create(std::string_view appName)`

Creates the Vulkan instance.

- `appName` is a UTF-8 `std::string_view`; it does not need to be null-terminated.
- The application name is stored in the resulting `Instance` as `std::string`.
- The API version is **not** taken from the caller. `vkEnumerateInstanceVersion` is called and its result is used as `VkApplicationInfo::apiVersion`. If the loader does not expose that function (Vulkan 1.0), `VK_API_VERSION_1_0` is used.
- After `vk::createInstance`, the instance-level dispatcher is initialized with `VULKAN_HPP_DEFAULT_DISPATCHER.init(instance)`.
- `VK_KHR_surface` and `VK_KHR_win32_surface` are enabled so that `createSurface()` works.

```cpp
auto instance = aru::rhi::Instance::create("Arunimesta");
if (!instance) {
    std::println("error: {}", instance.error());
    std::terminate();
}
```

### `Error`

| Value | Meaning |
| :--- | :--- |
| `None` | No error. |
| `DynamicLoaderFailed` | `vkGetInstanceProcAddr` could not be loaded. |
| `ApiVersionQueryFailed` | `vkEnumerateInstanceVersion` returned a failure code. |
| `InstanceCreationFailed` | `vk::createInstance` threw. |
| `SurfaceCreationFailed` | `vk::createWin32SurfaceKHR` threw or the window was empty. |
| `AdapterNotFound` | No physical device has a graphics + present queue family for the surface. |
| `DeviceCreationFailed` | `vk::createDevice` threw or the adapter was empty. |
| `QueueRequestFailed` | The requested queue flags are unavailable on the selected family. |

`Error` lives in the `aru.rhi:error` partition (`src/error.cppm`) together with its `std::formatter<Error, char>` specialization, so it can be printed with `std::println`. The `aru.rhi` facade re-exports that partition.

Failures are returned as `std::unexpected(Error::X)`.

### Factory members returning `std::expected<T, Error>`

| Function | Defined in | Description |
| :--- | :--- | :--- |
| `createSurface(const aru::window::Window&)` | `:surface` | Creates a Win32 Vulkan surface. |
| `requestAdapter(const Surface&)` | `:adapter` | Selects a physical device with graphics + present support. |

### Accessors

| Function | Description |
| :--- | :--- |
| `vk::Instance handle() const noexcept` | The underlying Vulkan instance handle. |
| `std::uint32_t apiVersion() const noexcept` | The version returned by `vkEnumerateInstanceVersion`. |
| `const std::string& applicationName() const noexcept` | Copy of the application name. |
| `bool empty() const noexcept` | `true` when no instance handle is held. |

### Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; `vk::Instance` is a raw handle. |
| Move constructor / move assignment | Steals the `vk::Instance` handle and the string, then empties the source. `noexcept`. |
| Destructor | Calls `Instance::destroy()` only when a handle is held. |

Only one object owns a given Vulkan handle; use `std::move` to transfer ownership or store `Instance` in a container.

## Build Notes

- `aru_rhi/CMakeLists.txt` registers `src/lib.cppm` and `src/instance.cppm` in a `FILE_SET CXX_MODULES`, links `Vulkan::Headers` publicly, and adds `include/` for `aru_rhi/vulkan.hpp`.
- The module is registered in the root `CMakeLists.txt`.

## Limitations

- No debug utils messenger yet.
- No validation-layer configuration.
