# `aru::rhi::CommandPool` API Reference

`CommandPool` is the Vulkan command-pool wrapper in the `aru.rhi:command_pool` module partition (`modules/aru_rhi/src/command_pool.cppm`). A pool belongs to one queue family and is the allocation source for command buffers.

## Creation

```cpp
auto pool = device.createCommandPool(queue);
if (!pool) {
    std::println("command pool: {}", pool.error());
    std::terminate();
}
```

`createCommandPool()` accepts a `Queue` from the same `Device` and the same queue family. It returns `std::unexpected(Error::CommandPoolCreationFailed)` when:

- the device or the queue is empty;
- the queue belongs to a different logical device or queue family;
- `vk::createCommandPool` throws.

The pool is created with `vk::CommandPoolCreateFlagBits::eResetCommandBuffer`, so individual command buffers can be reset with `CommandBuffer::reset()`. The error is formattable through `std::formatter<aru::rhi::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `vk::CommandPool handle() const noexcept` | Underlying command pool. |
| `vk::Device deviceHandle() const noexcept` | Owning device. |
| `std::uint32_t queueFamilyIndex() const noexcept` | Queue family the pool was created for. |
| `bool empty() const noexcept` | `true` when no pool handle is held. |
| `bool reset(vk::CommandPoolResetFlags flags = {}) noexcept` | Resets every command buffer allocated from this pool; `false` when empty or on failure. |
| `CommandPool copy() const` | Metadata-only copy (see below). |
| `CommandPool move() noexcept` | Transfers the handle; source becomes empty. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies the device and family index but not the `vk::CommandPool`; the copy is `empty()`. |
| Move constructor / move assignment / `move()` | Steals the handle and empties the source. `noexcept`. |
| Destructor | Calls `destroyCommandPool()` only when a handle is held. |

Destroy all command buffers allocated from the pool before destroying the pool. In `main.cpp` the declarations are `command_pool` then `command_buffers`, so reverse declaration order is correct.
