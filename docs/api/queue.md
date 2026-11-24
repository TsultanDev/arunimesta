# `renaru::Queue` API Reference

`Queue` is the queue wrapper in the `renaru:queue` module partition (`renaru/src/queue.cppm`). It is requested from a `Device` with a combination of `QueueFlags` and returned in a `std::expected<Queue, Error>`.

## `QueueFlags` and bitwise operators

`QueueFlags` and its operators live in `renaru:instance` (`src/instance.cppm`) so both devices and consumers can use them. The operators are `constexpr`, so combinations can be compile-time constants:

```cpp
constexpr auto sFlags = renaru::QueueFlags::Graphics | renaru::QueueFlags::Present;
auto queue = device.requestQueue(sFlags);
if (!queue) {
    std::println("queue: {}", queue.error());
    std::terminate();
}
```

| Name | Meaning |
| :--- | :--- |
| `None` | No flag. |
| `Graphics` | `VK_QUEUE_GRAPHICS_BIT`. |
| `Compute` | `VK_QUEUE_COMPUTE_BIT`. |
| `Transfer` | `VK_QUEUE_TRANSFER_BIT`. |
| `Present` | Surface presentation support (checked via `vkGetPhysicalDeviceSurfaceSupportKHR`). |

These are the spellings used by `main.cpp`.

| Operator | Description |
| :--- | :--- |
| `constexpr QueueFlags operator|`, `&`, `^`, `~` | Bitwise combinations. |
| `constexpr QueueFlags& operator|=` | In-place combination. |
| `constexpr bool hasFlag(QueueFlags value, QueueFlags flag)` | Tests a single flag. |

## Creation

`Device::requestQueue(sFlags)` checks the selected family against the requested bits (`Present` uses `vkGetPhysicalDeviceSurfaceSupportKHR`) and calls `vkGetDeviceQueue(device, family, 0, ...)`. `QueueRequestFailed` is returned as `std::unexpected(Error::QueueRequestFailed)` on any mismatch; it is formattable through `std::formatter<renaru::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `VkQueue handle() const noexcept` | Underlying `VkQueue`. |
| `VkDevice deviceHandle() const noexcept` | Device the queue belongs to. |
| `QueueFlags flags() const noexcept` | Flags requested at creation. |
| `std::uint32_t familyIndex() const noexcept` | Queue family index. |
| `bool empty() const noexcept` | `true` when no queue handle is held. |
| `std::expected<void, Error> submit(const CommandBuffer& sBuffer) const` | Submits one primary buffer with the frame semaphores/fence (defined in `:command_buffer`). |
| `std::expected<void, Error> present() const` | Presents the current swapchain image after render completion (defined in `:command_buffer`). |

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; queues hold raw Vulkan handles even though they are non-owning. |
| Move constructor / move assignment | Steals the handles/metadata and empties the source. `noexcept`. |
| Destructor | Defaulted; queues are destroyed with the `Device`. |

The `VkDevice` stored in a `Queue` is borrowed. Destroy the `Queue` before its `Device`. For the full submit/present call chain, see [frame.md](frame.md).
