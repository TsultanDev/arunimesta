# `aru::rhi::Swapchain` API Reference

`Swapchain` is the Vulkan swapchain wrapper in the `aru.rhi:swapchain` module partition (`aru_rhi/src/swapchain.cppm`). It is created by `Device` from the surface and the window size.

## Creation

```cpp
auto swapchain = device.createSwapchain(window.windowWidth(), window.windowHeight());
if (!swapchain) {
    std::println("swapchain: {}", swapchain.error());
    std::terminate();
}
```

`createSwapchain(width, height)`:

- Reads surface capabilities, formats, and present modes from the physical device.
- Prefers `B8G8R8A8_SRGB` + `SRGB_NONLINEAR`; falls back to the first available format.
- Prefers mailbox presentation; falls back to FIFO (which is always available).
- Uses `currentExtent` when defined, otherwise clamps the requested width/height to the surface limits.
- Requests `minImageCount + 1` images, capped by `maxImageCount`.
- Enables `COLOR_ATTACHMENT` usage and opaque composite alpha.

`SwapchainCreationFailed` is returned as `std::unexpected(Error::SwapchainCreationFailed)` on any query failure or `vk::createSwapchainKHR` exception. It is formattable through `std::formatter<aru::rhi::Error, char>`.

## Swapchain Images

```cpp
auto images = device.createSwapchainImages(*swapchain);
if (!images) {
    std::println("images: {}", images.error());
    std::terminate();
}
```

`*swapchain` is the `Swapchain` value held by the `std::expected` from the creation step above. The call returns `std::expected<std::vector<Resource>, Error>`; on success `images` holds a `std::vector<Resource>`. Each `Resource` contains a swapchain `vk::Image` (borrowed from the swapchain) and an owned `vk::ImageView`. Use these resources to create framebuffers. See [`api/resource.md`](resource.md).

## API

| Function | Description |
| :--- | :--- |
| `vk::SwapchainKHR handle() const noexcept` | Underlying swapchain. |
| `vk::Device deviceHandle() const noexcept` | Owning logical device. |
| `vk::Format format() const noexcept` | Chosen image format. |
| `vk::Extent2D extent() const noexcept` | Chosen image extent. |
| `std::uint32_t imageCount() const noexcept` | Number of images requested. |
| `bool empty() const noexcept` | `true` when no swapchain handle is held. |
| `Swapchain copy() const` | Metadata-only copy (see below). |
| `Swapchain move() noexcept` | Transfers the handle; source becomes empty. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies device, format, extent, and image count, but **not** the `vk::SwapchainKHR`; the copy is `empty()`. |
| Move constructor / move assignment / `move()` | Steals the swapchain and resets the source. `noexcept`. |
| Destructor | Calls `destroySwapchainKHR()` only when a handle is held. |

Destroy swapchain image views before the swapchain itself. In `main.cpp`, the `Resource` vector is declared after `swapchain`, so its destructor runs first.
