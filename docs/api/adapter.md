# `renaru::Adapter` API Reference

`Adapter` is the physical-device wrapper in the `renaru:adapter` module partition (`renaru/src/adapter.cppm`). It is selected from an `Instance` and a `Surface` and returned in a `std::expected<Adapter, Error>`.

## Creation

There is no public constructor. Use the `Instance` member:

```cpp
auto adapter = instance.requestAdapter(surface);
if (!adapter) {
    std::println("adapter: {}", adapter.error());
    std::terminate();
}
```

`requestAdapter()` enumerates physical devices, then walks their queue families. The first family that has `VK_QUEUE_GRAPHICS_BIT` **and** supports presentation on the given surface wins. The `vk::Instance`, `vk::PhysicalDevice`, `vk::SurfaceKHR`, and family index are stored for later device/queue creation.

`AdapterNotFound` is returned as `std::unexpected(Error::AdapterNotFound)` when no physical device satisfies both conditions. It is formattable through `std::formatter<renaru::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `vk::PhysicalDevice handle() const noexcept` | Selected physical device. |
| `vk::Instance instanceHandle() const noexcept` | Parent instance. |
| `vk::SurfaceKHR surfaceHandle() const noexcept` | Surface used for the present-support test. |
| `std::uint32_t queueFamilyIndex() const noexcept` | Graphics + present queue family index. |
| `bool empty() const noexcept` | `true` when no physical device is held. |
| `std::expected<Device, Error> createDevice(const Adapter& adapter) const` | Creates a logical device (defined in `:device`). |

The `createDevice(const Adapter&)` parameter echoes the call style in `main.cpp` (`adapter.createDevice(adapter)`); the object itself already carries the needed handles.

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; the adapter holds raw Vulkan handles. |
| Move constructor / move assignment | Steals all handles and the family index; source is emptied. `noexcept`. |
| Destructor | Defaulted; the adapter owns no Vulkan resource. |

The physical device and surface are borrowed from `Instance`/`Surface`. Keep those objects alive for the adapter's lifetime.
