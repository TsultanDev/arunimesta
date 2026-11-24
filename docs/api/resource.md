# `renaru::Resource` API Reference

`Resource` is a tagged union in the `renaru:resource` module partition (`renaru/src/resource.cppm`). It holds either an image plus its image view, or a buffer plus its buffer view.

## Union Layout

```cpp
enum class ResourceKind : std::uint8_t {
    None = 0,
    Image,
    Buffer,
};

// Conceptually:
union {
    struct { VkImage image;  VkImageView view; };  // ResourceKind::Image
    struct { VkBuffer buffer; VkBufferView view; }; // ResourceKind::Buffer
};
```

The implementation uses named private structs `ImageData` and `BufferData` inside a `Data` union, plus a `ResourceKind` tag and an ownership flag.

## Creation

Resources are created through `Device`:

```cpp
auto imageResource = device.createImageResource(simage, sformat, sextent);
if (!imageResource) {
    std::println("image resource: {}", imageResource.error());
    std::terminate();
}

auto bufferResource = device.createBufferResource(
    256,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VK_FORMAT_R32_SFLOAT
);
if (!bufferResource) {
    std::println("buffer resource: {}", bufferResource.error());
    std::terminate();
}
```

`createImageResource()` creates a 2D `VkImageView` with a color subresource range. The `VkImage` itself is borrowed (typically from a swapchain); the view is owned.

`createBufferResource()` creates an exclusive `VkBuffer` and a `VkBufferView` over the whole buffer. Both are owned. `ResourceCreationFailed` is returned as `std::unexpected(Error::ResourceCreationFailed)` on failure; it is formattable through `std::formatter<renaru::Error, char>`.

`Device::createSwapchainImages()` uses `createImageResource()` internally, so callers normally receive image resources rather than creating them directly. It returns `std::expected<std::vector<Resource>, Error>`.

## API

| Function | Description |
| :--- | :--- |
| `ResourceKind kind() const noexcept` | Which union member is active. |
| `bool empty() const noexcept` | `true` when no image/image view or buffer is held. |
| `VkDevice deviceHandle() const noexcept` | Device that owns the views/buffers. |
| `VkImage imageHandle() const noexcept` | Image, or `{}` when not an image resource. |
| `VkImageView imageViewHandle() const noexcept` | Image view, or `{}` when not an image resource. |
| `VkBuffer bufferHandle() const noexcept` | Buffer, or `{}` when not a buffer resource. |
| `VkBufferView bufferViewHandle() const noexcept` | Buffer view, or `{}` when not a buffer resource. |
| `VkExtent2D extent() const noexcept` | Extent stored for image resources; used by framebuffer creation. |
| `VkDeviceSize bufferSize() const noexcept` | Size stored for buffer resources. |

## Move-Only Semantics

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment | Deleted; resource handles are raw Vulkan handles. |
| Move constructor / move assignment | Steals the active union member, metadata, and ownership flag; source becomes empty. `noexcept`. |
| Destructor | Destroys only owned handles: `VkImageView` for images; `VkBufferView` then `VkBuffer` for buffers. |

All engine resource wrappers are move-only. Iterate swapchain images by `const auto&` in `main.cpp`; transfer ownership with `std::move` when storing them.

## Limitations

- No mipmaps or array layers beyond one base level/layer in the created image view.
- No buffer view format validation beyond what Vulkan performs.
- No memory allocation/binding yet; `createBufferResource()` creates a buffer handle without device memory.
