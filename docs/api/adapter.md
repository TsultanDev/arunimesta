# `renaru::Adapter` API Reference

`Adapter` is the physical-device wrapper in the `renaru:adapter` module partition (`renaru/src/adapter.cppm`). It is selected from an `Instance` and a `Surface` and returned in a `std::expected<Adapter, Error>`.

## Creation

There is no public constructor. Use the `Instance` member:

```cpp
auto adapter = instance.requestAdapter(surface);
if (!adapter) {
    std::println("adapter: {}", static_cast<std::uint32_t>(adapter.error()));
    std::terminate();
}
```

`requestAdapter()` enumerates physical devices, then walks their queue families. The first family that has `VK_QUEUE_GRAPHICS_BIT` **and** supports presentation on the given surface wins. The `VkInstance`, `VkPhysicalDevice`, `VkSurfaceKHR`, and family index are stored for later device/queue creation.

`eAdapterNotFound` is returned as `std::unexpected(Error::eAdapterNotFound)` when no physical device satisfies both conditions. It is printed as a numeric value.

## API

| Function | Description |
| :--- | :--- |
| `VkPhysicalDevice handle() const noexcept` | Selected physical device. |
| `VkInstance instanceHandle() const noexcept` | Parent instance. |
| `VkSurfaceKHR surfaceHandle() const noexcept` | Surface used for the present-support test. |
| `std::uint32_t queueFamilyIndex() const noexcept` | Graphics + present queue family index. |
| `bool empty() const noexcept` | `true` when no physical device is held. |
| `std::expected<Device, Error> createDevice(const Adapter& sAdapter) const` | Creates a logical device (defined in `:device`). |

The `createDevice(const Adapter& sAdapter)` parameter echoes the call style in `main.cpp` (`adapter.createDevice(adapter)`); the object itself already carries the needed handles.

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; the adapter holds raw Vulkan handles. |
| Move constructor / move assignment | Steals all handles and the family index; source is emptied. `noexcept`. |
| Destructor | Defaulted; the adapter owns no Vulkan resource. |

The physical device and surface are borrowed from `Instance`/`Surface`. Keep those objects alive for the adapter's lifetime.
