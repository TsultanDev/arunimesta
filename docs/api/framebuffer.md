# `renaru::Framebuffer` API Reference

`Framebuffer` is the Vulkan framebuffer wrapper in the `renaru:framebuffer` module partition (`renaru/src/framebuffer.cppm`). One framebuffer is created per swapchain image.

## Creation

```cpp
auto framebuffer = device.createFramebuffer(renderPass, image);
if (!framebuffer) {
    std::println("framebuffer: {}", framebuffer.error());
    std::terminate();
}
```

`image` must be an image `Resource` (`ResourceKind::Image`) with a non-null image view and a non-zero extent. The framebuffer uses:

- `renderPass.handle()` as its render pass.
- `image.imageViewHandle()` as its single attachment.
- `image.extent().width` / `image.extent().height` as its dimensions.
- One layer.

`FramebufferCreationFailed` is returned as `std::unexpected(Error::FramebufferCreationFailed)` on invalid input or `vk::createFramebuffer` failure; it is formattable through `std::formatter<renaru::Error, char>`.

### Typical loop

```cpp
auto images = device.createSwapchainImages(swapchain);
if (!images) {
    std::println("images: {}", images.error());
    std::terminate();
}
auto renderPass = device.createRenderPass(swapchain);
if (!renderPass) {
    std::println("render pass: {}", renderPass.error());
    std::terminate();
}

std::vector<renaru::Framebuffer> framebuffers;
framebuffers.reserve(images->size());

for (const auto& image : *images) {
    auto framebuffer = device.createFramebuffer(*renderPass, image);
    if (!framebuffer) {
        std::println("framebuffer: {}", framebuffer.error());
        std::terminate();
    }
    framebuffers.push_back(std::move(*framebuffer)); // transfer ownership into the vector
}
```

`createSwapchainImages()` yields a `std::vector<Resource>` inside a `std::expected`. Use `std::move` when pushing engine-owned objects into a `std::vector`; these classes have no `copy()` / `move()` member functions.

## API

| Function | Description |
| :--- | :--- |
| `vk::Framebuffer handle() const noexcept` | Underlying framebuffer. |
| `vk::Device deviceHandle() const noexcept` | Owning device. |
| `std::uint32_t width() const noexcept` | Framebuffer width. |
| `std::uint32_t height() const noexcept` | Framebuffer height. |
| `bool empty() const noexcept` | `true` when no framebuffer handle is held. |

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; `vk::Framebuffer` is a raw handle. |
| Move constructor / move assignment | Steals the framebuffer and empties the source. `noexcept`. |
| Destructor | Calls `destroyFramebuffer()` only when a handle is held. |

Destroy framebuffers before their render pass and device. In `main.cpp`, declaration order gives the correct reverse destruction order: `framebuffers`, `render_pass`, `swapchain_images`, `swapchain`, `device`.
