# `renaru::RenderPass` API Reference

`RenderPass` is the Vulkan render pass wrapper in the `renaru:render_pass` module partition (`renaru/src/render_pass.cppm`). It is created from a `Swapchain`.

## Creation

```cpp
auto renderPass = device.createRenderPass(swapchain);
if (!renderPass) {
    std::println("render pass: {}", renderPass.error());
    std::terminate();
}
```

The current render pass has:

- One color attachment using `swapchain.format()`, `SAMPLES_1`, clear-on-load, store-on-store, initial layout `UNDEFINED`, final layout `PRESENT_SRC_KHR`.
- One graphics subpass referencing that attachment as `COLOR_ATTACHMENT_OPTIMAL`.
- One external dependency on `COLOR_ATTACHMENT_OUTPUT` with `COLOR_ATTACHMENT_WRITE` destination access.

`RenderPassCreationFailed` is returned as `std::unexpected(Error::RenderPassCreationFailed)` when the device/swapchain is empty or `vkCreateRenderPass` fails. It is formattable through `std::formatter<renaru::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `template<typename Func> std::expected<void, Error> render(Func&& callback) const` | Begins the render pass with the stored clear color, runs the callback, then ends the pass. |
| `template<typename Func> std::expected<void, Error> render(std::array<float, 4> clearColor, Func&& callback) const` | Same, with an explicit RGBA clear color. |
| `void setClearColor(std::array<float, 4> color) noexcept` | Sets the stored clear color (default `{0.3f, 0.3f, 0.3f, 1.0f}`). |
| `std::array<float, 4> clearColor() const noexcept` | Returns the stored clear color. |
| `VkRenderPass handle() const noexcept` | Underlying render pass. |
| `VkDevice deviceHandle() const noexcept` | Owning device. |
| `bool empty() const noexcept` | `true` when no render pass handle is held. |

`render()` is meant to be called inside [`CommandBuffer::record()`](command_buffer.md): it uses the active command buffer, the framebuffer for `Device::currentImageIndex()`, and the cached swapchain extent. It returns `RenderPassRecordingFailed` when any of those is missing or when Vulkan rejects `vkCmdBeginRenderPass` / `vkCmdEndRenderPass`. See [frame.md](frame.md) for the full flow.

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; `VkRenderPass` is a raw handle. |
| Move constructor / move assignment | Steals the render pass and empties the source. `noexcept`. |
| Destructor | Calls `vkDestroyRenderPass` only when a handle is held. |

A `RenderPass` must be destroyed before the `Device` that created it.
