# `aru::rhi::Resource` API Reference

`Resource` is a tagged union in the `aru.rhi:resource` module partition (`aru_rhi/src/resource.cppm`). It holds either an image plus its image view, or a buffer plus its buffer view.

## Union Layout

```cpp
enum class ResourceKind : std::uint8_t {
    None = 0,
    Image,
    Buffer,
};

// Conceptually:
union {
    struct { vk::Image image;  vk::ImageView view; };  // ResourceKind::Image
    struct { vk::Buffer buffer; vk::BufferView view; }; // ResourceKind::Buffer
};
```

The implementation uses named private structs `ImageData` and `BufferData` inside a `Data` union, plus a `ResourceKind` tag and an ownership flag.

## Creation

Resources are created through `Device`:

```cpp
auto imageResource = device.createImageResource(image, format, extent);
if (!imageResource) {
    std::println("image resource: {}", imageResource.error());
    std::terminate();
}

auto bufferResource = device.createBufferResource(
    256,
    vk::BufferUsageFlagBits::eUniformBuffer,
    vk::Format::eR32Sfloat
);
if (!bufferResource) {
    std::println("buffer resource: {}", bufferResource.error());
    std::terminate();
}
```

`createImageResource()` creates a 2D `vk::ImageView` with a color subresource range. The `vk::Image` itself is borrowed (typically from a swapchain); the view is owned.

`createBufferResource()` creates an exclusive `vk::Buffer` and a `vk::BufferView` over the whole buffer. Both are owned. `ResourceCreationFailed` is returned as `std::unexpected(Error::ResourceCreationFailed)` on failure; it is formattable through `std::formatter<aru::rhi::Error, char>`.

`Device::createSwapchainImages()` uses `createImageResource()` internally, so callers normally receive image resources rather than creating them directly. It returns `std::expected<std::vector<Resource>, Error>`.

## API

| Function | Description |
| :--- | :--- |
| `ResourceKind kind() const noexcept` | Which union member is active. |
| `bool empty() const noexcept` | `true` when no image/image view or buffer is held. |
| `vk::Device deviceHandle() const noexcept` | Device that owns the views/buffers. |
| `vk::Image imageHandle() const noexcept` | Image, or `{}` when not an image resource. |
| `vk::ImageView imageViewHandle() const noexcept` | Image view, or `{}` when not an image resource. |
| `vk::Buffer bufferHandle() const noexcept` | Buffer, or `{}` when not a buffer resource. |
| `vk::BufferView bufferViewHandle() const noexcept` | Buffer view, or `{}` when not a buffer resource. |
| `vk::Extent2D extent() const noexcept` | Extent stored for image resources; used by framebuffer creation. |
| `vk::DeviceSize bufferSize() const noexcept` | Size stored for buffer resources. |
| `Resource copy() const` | Copies handles as non-owning references. |
| `Resource move() noexcept` | Transfers ownership; source becomes empty. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Copies the active union member and metadata, but sets the ownership flag to `false`. The copy keeps usable handles without double-destroying. |
| Move constructor / move assignment / `move()` | Steals the active member, metadata, and ownership flag; source becomes empty. `noexcept`. |
| Destructor | Destroys only owned handles: `ImageView` for images; `BufferView` then `Buffer` for buffers. |

This differs from `Swapchain`/`Framebuffer`, whose copies are `empty()`. It allows `for (const auto& image : swapchain_images)` and passing `Resource` by value without risking a double destroy.

## Limitations

- No mipmaps or array layers beyond one base level/layer in the created image view.
- No buffer view format validation beyond what Vulkan performs.
- No memory allocation/binding yet; `createBufferResource()` creates a buffer handle without device memory.
