# `renaru::CommandBuffer` API Reference

`CommandBuffer` is the Vulkan command-buffer wrapper in the `renaru:command_buffer` module partition (`renaru/src/command_buffer.cppm`). Command buffers are allocated in one batch from a `CommandPool` and returned as `std::vector<CommandBuffer>` inside a `std::expected`.

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
- `vkAllocateCommandBuffers` fails or returns a different number of buffers.

The error is formattable through `std::formatter<renaru::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `VkCommandBuffer handle() const noexcept` | Underlying command buffer. |
| `VkDevice deviceHandle() const noexcept` | Owning device. |
| `VkCommandPool poolHandle() const noexcept` | Pool this buffer was allocated from. |
| `bool empty() const noexcept` | `true` when no command-buffer handle is held. |
| `template<typename Func> std::expected<void, Error> record(Func&& callback)` | Resets the buffer, calls `vkBeginCommandBuffer`, runs the callback, then calls `vkEndCommandBuffer`. |
| `bool begin(VkCommandBufferUsageFlags flags = {}) noexcept` | Starts recording; `false` when empty or on failure. |
| `void end() noexcept` | Ends recording; silently ignores an empty buffer or a Vulkan error. |
| `bool reset(VkCommandBufferResetFlags flags = {}) noexcept` | Resets the buffer for reuse; requires the pool's `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` flag. |

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; `VkCommandBuffer` is a raw handle. |
| Move constructor / move assignment | Steals the handle and empties the source. `noexcept`. |
| Destructor | Calls `vkFreeCommandBuffers` for its own handle when non-null. |

## Ownership and Lifetime

A `CommandBuffer` frees its own handle, while the `CommandPool` frees the pool itself. Keep the pool alive longer than every buffer allocated from it. In `main.cpp` the buffers are declared after the pool, so they are destroyed first.

Use `std::move` when transferring buffers into a container:

```cpp
auto result = device.allocateCommandBuffers(pool, swapchainImages.size());
if (!result) {
    std::println("command buffers: {}", result.error());
    std::terminate();
}

std::vector<renaru::CommandBuffer> buffers = std::move(*result);
check(buffers[0].record([&]() {
    check(renderPass.render([]() {
        // draw commands will be added here
    }));
}));
```

`record()` is the preferred prototype API; direct `begin()` / `end()` / `reset()` remain available for manual control. See [frame.md](frame.md) for the full loop. Drawing commands themselves are still future work; see [roadmap.md](../roadmap.md).
