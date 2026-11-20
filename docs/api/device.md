# `aru::rhi::Device` API Reference

`Device` is the logical-device wrapper in the `aru.rhi:device` module partition (`modules/aru_rhi/src/device.cppm`). It is created from an `Adapter` and returned in an `aru::standard::Result`.

## Creation

```cpp
auto device = adapter.createDevice(adapter)
    .valueOrUnexpected([](aru::rhi::Error&& err) {
        aru::standard::println("device: {}", err);
    });
```

`createDevice()` creates one queue from the adapter's graphics + present family and enables `VK_KHR_swapchain`. `DeviceCreationFailed` is returned if either adapter is empty or `vk::createDevice` throws; it is formattable through `std::formatter<aru::rhi::Error, char>`.

The object stores the parent instance, physical device, surface, logical device, and queue family index so that `requestQueue()` can validate and fetch queues.

## API

| Function | Description |
| :--- | :--- |
| `vk::Device handle() const noexcept` | Underlying logical device. |
| `vk::PhysicalDevice physicalDeviceHandle() const noexcept` | Parent physical device. |
| `vk::Instance instanceHandle() const noexcept` | Parent instance. |
| `vk::SurfaceKHR surfaceHandle() const noexcept` | Surface associated with this device. |
| `std::uint32_t queueFamilyIndex() const noexcept` | Queue family used at creation. |
| `bool empty() const noexcept` | `true` when no logical device is held. |
| `Result<Queue, Error> requestQueue(QueueFlags flags) const` | Fetches a queue satisfying the requested flags (defined in `:queue`). |
| `Device copy() const` | Metadata-only copy (see below). |
| `Device move() noexcept` | Transfers the device handle; source becomes empty. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies instance, physical device, surface, and family index, but **not** the `vk::Device`; the copy is `empty()`. |
| Move constructor / move assignment / `move()` | Steals all handles and the family index; source is emptied. `noexcept`. |
| Destructor | Calls `destroy()` only when a `vk::Device` handle is held, then clears the borrowed handles. |

Only one object owns a logical device. The surface and instance are borrowed; keep `Surface` and `Instance` alive longer than `Device`.
