# Arunimesta Documentation

This folder contains the technical documentation for the Arunimesta project. The documents are written based on the current state of the code; if the implementation changes, update the relevant documents.

## Document List

| Document | Description |
| :--- | :--- |
| [architecture.md](architecture.md) | Module architecture, pimpl pattern, ownership, and initialization flow. |
| [building.md](building.md) | Prerequisites, building with presets or manually, and troubleshooting. |
| [coding-style.md](coding-style.md) | Naming conventions, `.cppm` file layout, and factory/pimpl patterns. |
| [api/application.md](api/application.md) | `aru::Application` API reference. |
| [api/array.md](api/array.md) | `aru::standard::Array<T, N>` API reference. |
| [api/string.md](api/string.md) | `aru::standard::String` (UTF-8/16/32) API reference. |
| [api/vector.md](api/vector.md) | `aru::standard::Vector<T>` API reference. |
| [api/result.md](api/result.md) | `aru::standard::Result<T, E>` API reference. |
| [api/print.md](api/print.md) | `aru::standard::println` / `eprintln` API reference. |
| [api/window.md](api/window.md) | `aru::window::Window` API reference (active). |
| [api/instance.md](api/instance.md) | `aru::rhi::Instance` API reference (Vulkan instance, dynamic dispatch). |
| [api/surface.md](api/surface.md) | `aru::rhi::Surface` API reference (Win32 Vulkan surface). |
| [api/adapter.md](api/adapter.md) | `aru::rhi::Adapter` API reference (physical device selection). |
| [api/device.md](api/device.md) | `aru::rhi::Device` API reference (logical device). |
| [api/queue.md](api/queue.md) | `aru::rhi::Queue` API reference and `QueueFlags` bitwise operators. |
| [roadmap.md](roadmap.md) | Status, milestones, and development plans. |

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
