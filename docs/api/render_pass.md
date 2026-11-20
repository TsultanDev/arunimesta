# `aru::rhi::RenderPass` API Reference

`RenderPass` is the Vulkan render pass wrapper in the `aru.rhi:render_pass` module partition (`modules/aru_rhi/src/render_pass.cppm`). It is created from a `Swapchain`.

## Creation

```cpp
auto renderPass = device.createRenderPass(swapchain)
    .valueOrUnexpected([](aru::rhi::Error&& err) {
        aru::standard::println("render pass: {}", err);
    });
```

The current render pass has:

- One color attachment using `swapchain.format()`, `SAMPLES_1`, clear-on-load, store-on-store, initial layout `UNDEFINED`, final layout `PRESENT_SRC_KHR`.
- One graphics subpass referencing that attachment as `COLOR_ATTACHMENT_OPTIMAL`.
- One external dependency on `COLOR_ATTACHMENT_OUTPUT` with `COLOR_ATTACHMENT_WRITE` destination access.

`RenderPassCreationFailed` is returned when the device/swapchain is empty or `vk::createRenderPass` throws. It is formattable through `std::formatter<aru::rhi::Error, char>`.

## API

| Function | Description |
| :--- | :--- |
| `vk::RenderPass handle() const noexcept` | Underlying render pass. |
| `vk::Device deviceHandle() const noexcept` | Owning device. |
| `bool empty() const noexcept` | `true` when no render pass handle is held. |
| `RenderPass copy() const` | Metadata-only copy (see below). |
| `RenderPass move() noexcept` | Transfers the handle; source becomes empty. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies the device handle but not the `vk::RenderPass`; the copy is `empty()`. |
| Move constructor / move assignment / `move()` | Steals the render pass and empties the source. `noexcept`. |
| Destructor | Calls `destroyRenderPass()` only when a handle is held. |

A `RenderPass` must be destroyed before the `Device` that created it.
