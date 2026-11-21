# Arunimesta Documentation

This folder contains the technical documentation for the Arunimesta project. The documents are written based on the current state of the code; if the implementation changes, update the relevant documents.

## Document List

| Document | Description |
| :--- | :--- |
| [architecture.md](architecture.md) | Module architecture, pimpl pattern, ownership, and initialization flow. |
| [assets.md](assets.md) | Logo, screenshots, and asset usage rules. |
| [building.md](building.md) | Prerequisites, building with presets or manually, and troubleshooting. |
| [coding-style.md](coding-style.md) | Naming conventions, `.cppm` file layout, and factory/pimpl patterns. |
| [api/application.md](api/application.md) | `aru::Application` API reference. |
| [api/window.md](api/window.md) | `aru::window::Window` API reference (active). |
| [api/instance.md](api/instance.md) | `aru::rhi::Instance` API reference (Vulkan instance, dynamic dispatch). |
| [api/surface.md](api/surface.md) | `aru::rhi::Surface` API reference (Win32 Vulkan surface). |
| [api/adapter.md](api/adapter.md) | `aru::rhi::Adapter` API reference (physical device selection). |
| [api/device.md](api/device.md) | `aru::rhi::Device` API reference (logical device). |
| [api/queue.md](api/queue.md) | `aru::rhi::Queue` API reference and `QueueFlags` bitwise operators. |
| [api/swapchain.md](api/swapchain.md) | `aru::rhi::Swapchain` API reference (format/extent/image selection). |
| [api/resource.md](api/resource.md) | `aru::rhi::Resource` tagged union (image + view / buffer + view). |
| [api/render_pass.md](api/render_pass.md) | `aru::rhi::RenderPass` API reference. |
| [api/framebuffer.md](api/framebuffer.md) | `aru::rhi::Framebuffer` API reference and per-image creation loop. |
| [api/command_pool.md](api/command_pool.md) | `aru::rhi::CommandPool` API reference (queue-family pool, reset). |
| [api/command_buffer.md](api/command_buffer.md) | `aru::rhi::CommandBuffer` API reference (batch allocation, record lifecycle). |
| [roadmap.md](roadmap.md) | Status, milestones, and development plans. |

## Community

| Document | Description |
| :--- | :--- |
| [CONTRIBUTING.md](../CONTRIBUTING.md) | Build, coding style, testing, and pull request workflow. |
| [CODE_OF_CONDUCT.md](../CODE_OF_CONDUCT.md) | Community expectations and enforcement. |
| [SECURITY.md](../SECURITY.md) | Private vulnerability reporting and disclosure policy. |
| [LICENSE](../LICENSE) | MIT License. |

## Documentation Principles

- **In sync with the code**: examples and API names must match the source in `modules/`.
- **Honest about status**: parts that are not implemented are explicitly marked.
- **Practical**: each document answers the questions that come up when building or developing the engine.
- **Language**: English, using standard technical terminology.

## Where to Start?

- Building for the first time? Read [building.md](building.md).
- Want to understand the code structure? Read [architecture.md](architecture.md).
- Want to add a module or change the API? Read [coding-style.md](coding-style.md).
- Want to know the next steps? Read [roadmap.md](roadmap.md).
