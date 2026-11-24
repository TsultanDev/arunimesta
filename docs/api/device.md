# `renaru::Device` API Reference

`Device` is the logical-device wrapper in the `renaru:device` module partition (`renaru/src/device.cppm`). It is created from an `Adapter` and returned in a `std::expected<Device, Error>`.

## Creation

```cpp
auto device = adapter.createDevice(adapter);
if (!device) {
    std::println("device: {}", device.error());
    std::terminate();
}
```

`createDevice()` creates one queue from the adapter's graphics + present family and enables `VK_KHR_swapchain`. `DeviceCreationFailed` is returned as `std::unexpected(Error::DeviceCreationFailed)` if either adapter is empty or `vkCreateDevice` fails; it is formattable through `std::formatter<renaru::Error, char>`.

The object stores the parent instance, physical device, surface, logical device, and queue family index so that `requestQueue()` can validate and fetch queues. It also caches the swapchain, per-image framebuffers, and frame synchronization state (fences, semaphores, `frame_index`) used by `acquire()`; see [frame.md](frame.md).

## API

| Function | Description |
| :--- | :--- |
| `VkDevice handle() const noexcept` | Underlying logical device. |
| `VkPhysicalDevice physicalDeviceHandle() const noexcept` | Parent physical device. |
| `VkInstance instanceHandle() const noexcept` | Parent instance. |
| `VkSurfaceKHR surfaceHandle() const noexcept` | Surface associated with this device. |
| `std::uint32_t queueFamilyIndex() const noexcept` | Queue family used at creation. |
| `std::uint32_t currentImageIndex() const noexcept` | Swapchain image acquired for the current frame. |
| `std::uint32_t frameIndex() const noexcept` | Current in-flight slot (0..`MaxFramesInFlight - 1`). |
| `static constexpr std::uint32_t MaxFramesInFlight` | Number of fence/semaphore slots (2). |
| `bool empty() const noexcept` | `true` when no logical device is held. |
| `void waitIdle() const noexcept` | Blocks until outstanding GPU work finishes; useful before destroying frame sync state. |
| `std::expected<Queue, Error> requestQueue(QueueFlags sFlags) const` | Fetches a queue satisfying the requested flags (defined in `:queue`). |
| `std::expected<CommandPool, Error> createCommandPool(const Queue& sQueue) const` | Creates a resettable command pool for the queue's family (defined in `:command_pool`). |
| `std::expected<std::vector<CommandBuffer>, Error> allocateCommandBuffers(const CommandPool& sCommandPool, std::uint32_t sCount) const` | Allocates `sCount` primary command buffers from the pool (defined in `:command_buffer`). |
| `std::expected<void, Error> acquire(Func&& callback)` | Waits/resets the in-flight fence, acquires the next swapchain image, runs the callback, then advances `frame_index` (see [frame.md](frame.md)). |

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; `VkDevice` and the sync objects are raw handles. |
| Move constructor / move assignment | Steals all owned handles, sync vectors, and the family index; source is emptied. `noexcept`. |
| Destructor | Destroys sync objects, then calls `vkDestroyDevice` only when a `VkDevice` handle is held, and clears the borrowed handles. |

Only one object owns a logical device. The surface and instance are borrowed; keep `Surface` and `Instance` alive longer than `Device`.
