# `renaru::Surface` API Reference

`Surface` is the Win32 Vulkan surface wrapper in the `renaru:surface` module partition (`renaru/src/surface.cppm`). It is created from an `Instance` and an existing `winaru::Window`, and returned in a `std::expected<Surface, Error>`.

## Creation

Surfaces are not created with a public constructor. Use the `Instance` member:

```cpp
auto instance = renaru::Instance::create("Arunimesta");
if (!instance) {
    std::println("instance: {}", instance.error());
    std::terminate();
}

auto surface = instance->createSurface(window);
if (!surface) {
    std::println("surface: {}", surface.error());
    std::terminate();
}
```

`Instance::create()` enables `VK_KHR_surface` and `VK_KHR_win32_surface`, so `vkCreateWin32SurfaceKHR` is available through `entry().resolveInstance(...)`. The `HWND` comes from `Window::handle()`; `GetModuleHandleW(nullptr)` supplies the `HINSTANCE`.

`SurfaceCreationFailed` is returned as `std::unexpected(Error::SurfaceCreationFailed)` when the window is empty or `vkCreateWin32SurfaceKHR` fails. It is formattable through `std::formatter<renaru::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `VkSurfaceKHR handle() const noexcept` | The underlying Vulkan surface. |
| `VkInstance instanceHandle() const noexcept` | The instance that owns the surface. |
| `bool empty() const noexcept` | `true` when no surface handle is held. |

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; `VkSurfaceKHR` is a raw handle. |
| Move constructor / move assignment | Steals the `VkSurfaceKHR` handle and empties the source. `noexcept`. |
| Destructor | Calls `vkDestroySurfaceKHR` only when a handle is held. |

Only one object owns a given `VkSurfaceKHR`. The surface must be destroyed before its instance; keep `Surface` declared after `Instance` in scope so the destructor order is correct.

## Build Notes

- `renaru:surface` imports `winaru` and `renaru:instance`.
- `renaru` links `Vulkan::Headers` and `winaru`.
- The shared `renaru/vulkan.h` defines `VK_USE_PLATFORM_WIN32_KHR` for every translation unit. Dispatch is owned by `renaru::Entry` in `entry.cppm`; this partition resolves commands through `entry()`.

## Limitations

- No surface capability / format queries.
- Windows-only, matching the current `winaru` target.
