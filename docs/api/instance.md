# `aru::rhi::Instance` API Reference

`Instance` is the Vulkan instance wrapper in the `aru.rhi:instance` module partition (`modules/aru_rhi/src/instance.cppm`). It is created through a static factory and returned in an `aru::standard::Result`.

The module uses **Vulkan-Hpp dynamic dispatch**: `vkGetInstanceProcAddr` and the instance entry points are resolved at runtime with `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC`. No `vulkan-1.lib` link is required.

## Header / Module

```cpp
import aru.rhi;      // facade, re-exports aru.rhi:instance
import aru.standard; // String<T>, Result<T, E>
```

For additional `aru_rhi` translation units that need Vulkan types, include `aru_rhi/vulkan.hpp`. That header sets up dynamic dispatch and declares the dispatcher as `extern`. The single definition (main storage) lives in `src/instance.cppm`.

## API

### `template<character T> static Result<Instance, Error> create(const String<T>& appName)`

Creates the Vulkan instance.

- `appName` accepts `String<char8_t>`, `String<char16_t>`, or `String<char32_t>`. It is transcoded to UTF-8 before being passed as `VkApplicationInfo::pApplicationName`.
- The application name is stored in the resulting `Instance` as `String<char8_t>`.
- The API version is **not** taken from the caller. `vkEnumerateInstanceVersion` is called and its result is used as `VkApplicationInfo::apiVersion`. If the loader does not expose that function (Vulkan 1.0), `VK_API_VERSION_1_0` is used.
- After `vk::createInstance`, the instance-level dispatcher is initialized with `VULKAN_HPP_DEFAULT_DISPATCHER.init(instance)`.
- `VK_KHR_surface` and `VK_KHR_win32_surface` are enabled so that `createSurface()` works.

```cpp
auto name = aru::standard::String<char16_t>::from(u"Arunimesta");
auto result = aru::rhi::Instance::create(name);
auto instance = result.valueOrUnexpected([](aru::rhi::Error&& err) {
    aru::standard::println("error: {}", err);
});
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

`Error` has a `std::formatter<Error, char>` specialization, so it can be printed with `println()`.

### Factory members returning `Result<T, Error>`

| Function | Defined in | Description |
| :--- | :--- | :--- |
| `createSurface(const aru::window::Window&)` | `:surface` | Creates a Win32 Vulkan surface. |
| `requestAdapter(const Surface&)` | `:adapter` | Selects a physical device with graphics + present support. |

### Accessors

| Function | Description |
| :--- | :--- |
| `vk::Instance handle() const noexcept` | The underlying Vulkan instance handle. |
| `std::uint32_t apiVersion() const noexcept` | The version returned by `vkEnumerateInstanceVersion`. |
| `const String<char8_t>& applicationName() const noexcept` | UTF-8 copy of the application name. |
| `bool empty() const noexcept` | `true` when no instance handle is held. |

### Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Deep-copies `applicationName` and `apiVersion`. The copy does **not** own the Vulkan handle and is therefore `empty()`. |
| Move constructor / move assignment / `move()` | Steals the `vk::Instance` handle and the string, then empties the source. `noexcept`. |
| Destructor | Calls `Instance::destroy()` only when a handle is held. |

Copy exists so that `Instance` fits containers and the `Result` copying rules. Only one object owns a given Vulkan handle; use `move()` to transfer ownership.

## Build Notes

- `modules/aru_rhi/CMakeLists.txt` registers `src/lib.cppm` and `src/instance.cppm` in a `FILE_SET CXX_MODULES`, links `Vulkan::Headers` and `aru_std` publicly, and adds `include/` for `aru_rhi/vulkan.hpp`.
- The module is registered in the root `CMakeLists.txt` after `aru_std`.

## Limitations

- No debug utils messenger yet.
- No swapchain yet (only the instance, surface, adapter, device, and queue exist).
- No validation-layer configuration.
