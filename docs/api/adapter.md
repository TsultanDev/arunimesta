# `aru::rhi::Adapter` API Reference

`Adapter` is the physical-device wrapper in the `aru.rhi:adapter` module partition (`modules/aru_rhi/src/adapter.cppm`). It is selected from an `Instance` and a `Surface` and returned in an `aru::standard::Result`.

## Creation

There is no public constructor. Use the `Instance` member:

```cpp
auto adapter = instance.requestAdapter(surface)
    .valueOrUnexpected([](aru::rhi::Error&& err) {
        aru::standard::println("adapter: {}", err);
    });
```

`requestAdapter()` enumerates physical devices, then walks their queue families. The first family that has `VK_QUEUE_GRAPHICS_BIT` **and** supports presentation on the given surface wins. The `vk::Instance`, `vk::PhysicalDevice`, `vk::SurfaceKHR`, and family index are stored for later device/queue creation.

`AdapterNotFound` is returned when no physical device satisfies both conditions. It is formattable through `std::formatter<aru::rhi::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `vk::PhysicalDevice handle() const noexcept` | Selected physical device. |
| `vk::Instance instanceHandle() const noexcept` | Parent instance. |
| `vk::SurfaceKHR surfaceHandle() const noexcept` | Surface used for the present-support test. |
| `std::uint32_t queueFamilyIndex() const noexcept` | Graphics + present queue family index. |
| `bool empty() const noexcept` | `true` when no physical device is held. |
| `Result<Device, Error> createDevice(const Adapter& adapter) const` | Creates a logical device (defined in `:device`). |
| `Adapter copy() const` | Metadata-only copy (see below). |
| `Adapter move() noexcept` | Transfers the handles; source becomes empty. |

The `createDevice(const Adapter&)` parameter echoes the call style in `main.cpp` (`adapter.createDevice(adapter)`); the object itself already carries the needed handles.

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies only the instance handle. Physical device, surface, and family index are reset, so the copy is `empty()`. |
| Move constructor / move assignment / `move()` | Steals all handles and the family index; source is emptied. `noexcept`. |
| Destructor | Defaulted; the adapter owns no Vulkan resource. |

The physical device and surface are borrowed from `Instance`/`Surface`. Keep those objects alive for the adapter's lifetime.
