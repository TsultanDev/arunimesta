# `aru::rhi::Surface` API Reference

`Surface` is the Win32 Vulkan surface wrapper in the `aru.rhi:surface` module partition (`modules/aru_rhi/src/surface.cppm`). It is created from an `Instance` and an existing `aru::window::Window`, and returned in an `aru::standard::Result`.

## Creation

Surfaces are not created with a public constructor. Use the `Instance` member:

```cpp
auto name = aru::standard::String<char8_t>::from("Arunimesta");
auto instance = aru::rhi::Instance::create(name)
    .valueOrUnexpected([](aru::rhi::Error&& err) {
        aru::standard::println("instance: {}", err);
    });

auto surface = instance.createSurface(window)
    .valueOrUnexpected([](aru::rhi::Error&& err) {
        aru::standard::println("surface: {}", err);
    });
```

`Instance::create()` enables `VK_KHR_surface` and `VK_KHR_win32_surface`, so `createWin32SurfaceKHR()` is available. The `HWND` comes from `Window::handle()`; `GetModuleHandleW(nullptr)` supplies the `HINSTANCE`.

`SurfaceCreationFailed` is returned when the window is empty or `vk::createWin32SurfaceKHR` throws. It is formattable through `std::formatter<aru::rhi::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `vk::SurfaceKHR handle() const noexcept` | The underlying Vulkan surface. |
| `vk::Instance instanceHandle() const noexcept` | The instance that owns the surface. |
| `bool empty() const noexcept` | `true` when no surface handle is held. |
| `Surface copy() const` | Metadata-only copy (see below). |
| `Surface move() noexcept` | Transfers the handle; source becomes empty. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies the instance handle but **not** the surface handle; the copy is `empty()`. |
| Move constructor / move assignment / `move()` | Steals the `vk::SurfaceKHR` handle and empties the source. `noexcept`. |
| Destructor | Calls `destroySurfaceKHR()` only when a handle is held. |

Only one object owns a given `vk::SurfaceKHR`. The surface must be destroyed before its instance; keep `Surface` declared after `Instance` in scope so the destructor order is correct.

## Build Notes

- `aru.rhi:surface` imports `aru.standard`, `aru.window`, and `aru.rhi:instance`.
- `aru_rhi` links `Vulkan::Headers`, `aru_std`, and `aru_window`.
- The shared `aru_rhi/vulkan.hpp` defines `VK_USE_PLATFORM_WIN32_KHR` for every translation unit so the dynamic-dispatch loader layout stays identical; its storage is still defined only in `instance.cppm`.

## Limitations

- No swapchain yet.
- No surface capability / format queries.
- Windows-only, matching the current `aru.window` target.
