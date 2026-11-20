# `aru::rhi::Queue` API Reference

`Queue` is the queue wrapper in the `aru.rhi:queue` module partition (`modules/aru_rhi/src/queue.cppm`). It is requested from a `Device` with a combination of `QueueFlags` and returned in an `aru::standard::Result`.

## `QueueFlags` and bitwise operators

`QueueFlags` and its operators live in `aru.rhi:instance` (`src/instance.cppm`) so both devices and consumers can use them. The operators are `constexpr`, so combinations can be compile-time constants:

```cpp
constexpr auto flags = aru::rhi::QueueFlags::Graphics | aru::rhi::QueueFlags::Present;
auto queue = device.requestQueue(flags).valueOrUnexpected([](aru::rhi::Error&& err){
    aru::standard::println("queue: {}", err);
});
```

| Name | Meaning |
| :--- | :--- |
| `None` | No flag. |
| `Graphics` | `VK_QUEUE_GRAPHICS_BIT`. |
| `Compute` | `VK_QUEUE_COMPUTE_BIT`. |
| `Transfer` | `VK_QUEUE_TRANSFER_BIT`. |
| `Present` | Surface presentation support (checked via `getSurfaceSupportKHR`). |

These are the spellings used by `main.cpp`.

| Operator | Description |
| :--- | :--- |
| `constexpr QueueFlags operator|`, `&`, `^`, `~` | Bitwise combinations. |
| `constexpr QueueFlags& operator|=` | In-place combination. |
| `constexpr bool hasFlag(QueueFlags value, QueueFlags flag)` | Tests a single flag. |

## Creation

`Device::requestQueue(flags)` checks the selected family against the requested bits (`Present` uses `getSurfaceSupportKHR`) and calls `getQueue(family, 0)`. `QueueRequestFailed` is returned on any mismatch; it is formattable through `std::formatter<aru::rhi::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `vk::Queue handle() const noexcept` | Underlying `VkQueue`. |
| `vk::Device deviceHandle() const noexcept` | Device the queue belongs to. |
| `QueueFlags flags() const noexcept` | Flags requested at creation. |
| `std::uint32_t familyIndex() const noexcept` | Queue family index. |
| `bool empty() const noexcept` | `true` when no queue handle is held. |
| `Queue copy() const` | Copies the handle (non-owning). |
| `Queue move() noexcept` | Transfers the handle; source becomes empty. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies device, queue handle, flags, and family index. Vulkan queues are not destroyed by the application, so sharing is safe. |
| Move constructor / move assignment / `move()` | Steals the handles/metadata and empties the source. `noexcept`. |
| Destructor | Defaulted; queues are destroyed with the `Device`. |

The `vk::Device` stored in a `Queue` is borrowed. Destroy the `Queue` before its `Device`.
