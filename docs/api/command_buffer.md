# `aru::rhi::CommandBuffer` API Reference

`CommandBuffer` is the Vulkan command-buffer wrapper in the `aru.rhi:command_buffer` module partition (`aru_rhi/src/command_buffer.cppm`). Command buffers are allocated in one batch from a `CommandPool` and returned as `std::vector<CommandBuffer>` inside a `std::expected`.

## Allocation

```cpp
auto buffers = device.allocateCommandBuffers(pool, imageCount);
if (!buffers) {
    std::println("command buffers: {}", buffers.error());
    std::terminate();
}
```

`allocateCommandBuffers()` creates `count` **primary** command buffers. It returns `std::unexpected(Error::CommandBufferAllocationFailed)` when:

- the device or pool is empty, or `count` is zero;
- the pool belongs to a different logical device;
- `vk::allocateCommandBuffers` throws or returns a different number of buffers.

The error is formattable through `std::formatter<aru::rhi::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `vk::CommandBuffer handle() const noexcept` | Underlying command buffer. |
| `vk::Device deviceHandle() const noexcept` | Owning device. |
| `vk::CommandPool poolHandle() const noexcept` | Pool this buffer was allocated from. |
| `bool empty() const noexcept` | `true` when no command-buffer handle is held. |
| `template<typename Func> std::expected<void, Error> record(Func&& callback)` | Resets the buffer, calls `vkBeginCommandBuffer`, runs the callback, then calls `vkEndCommandBuffer`. |
| `bool begin(vk::CommandBufferUsageFlags flags = {}) noexcept` | Starts recording; `false` when empty or on failure. |
| `void end() noexcept` | Ends recording; silently ignores an empty buffer or a Vulkan error. |
| `bool reset(vk::CommandBufferResetFlags flags = {}) noexcept` | Resets the buffer for reuse; requires the pool's `eResetCommandBuffer` flag. |
| `CommandBuffer copy() const` | Metadata-only copy (see below). |
| `CommandBuffer move() noexcept` | Transfers the handle; source becomes empty. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies the owner pointer, device and pool handles, but not the `vk::CommandBuffer`; the copy is `empty()`. |
| Move constructor / move assignment / `move()` | Steals the handle and empties the source. `noexcept`. |
| Destructor | Calls `freeCommandBuffers()` for its own handle when non-null. |

## Ownership and Lifetime

A `CommandBuffer` frees its own handle, while the `CommandPool` frees the pool itself. Keep the pool alive longer than every buffer allocated from it. In `main.cpp` the buffers are declared after the pool, so they are destroyed first.

Use `move()` when transferring buffers into a container:

```cpp
auto result = device.allocateCommandBuffers(pool, swapchainImages.size());
if (!result) {
    std::println("command buffers: {}", result.error());
    std::terminate();
}

std::vector<aru::rhi::CommandBuffer> buffers = std::move(*result);
check(buffers[0].record([&]() {
    check(renderPass.render([]() {
        // draw commands will be added here
    }));
}));
```

`record()` is the preferred prototype API; direct `begin()` / `end()` / `reset()` remain available for manual control. See [frame.md](frame.md) for the full loop. Drawing commands themselves are still future work; see [roadmap.md](../roadmap.md).
