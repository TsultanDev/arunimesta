# API: `aru::window::Window`

Reference for the `Window` class exported by module `aru.window` and re-exported by `aru`.

```cpp
import aru;
// or: import aru.window;

aru::window::Window
```

## Overview

`Window` represents a Win32 window and its message loop. Instances are created through the `create_window()` factory and are *move-only*.

Implementation details are stored in `Window_T` (not exported) and owned through `std::unique_ptr`. The type is created with the Unicode Win32 APIs (`RegisterClassExW`, `CreateWindowExW`, `DefWindowProcW`, `PeekMessageW`, `DispatchMessageW`).

## API

### `static std::expected<Window, WindowError> create_window(std::string title, uint32_t width, uint32_t height)`

Creates, shows, and returns a window.

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `title` | `std::string` | UTF-8 window title. Converted to UTF-16 internally, so non-ASCII text is supported. |
| `width` | `uint32_t` | Window width in pixels. |
| `height` | `uint32_t` | Window height in pixels. |

Returns:

- A `std::expected` containing a `Window` on success.
- `std::unexpected(WindowError::...)` if the module handle, window class registration, or window creation fails.

Side effects (debug builds only):

- Prints `Aru: Window(created!!), title: <title>` to `stderr`.

The window class `ArunimestaClass` is registered on first use. Registration is idempotent: `ERROR_CLASS_ALREADY_EXISTS` is ignored, so the factory can be called more than once in a process.

Example:

```cpp
auto window = aru::window::Window::create_window("Arunimesta — 世界", 800, 600);
if (!window) {
    return 1;
}
```

### `bool event_loop()`

Pumps all pending Win32 messages without blocking and returns whether the loop should continue.

- Returns `false` after `WM_QUIT`, which is posted by `WM_DESTROY` when the window is closed.
- Returns `true` otherwise, including when there are no pending messages.

Call it in a loop:

```cpp
while (window->event_loop()) {
}
```

## Lifetime and Ownership

| Operation | Status | Notes |
| :--- | :--- | :--- |
| Constructor | Private | Use `create_window()` |
| Copy constructor | `= delete` | Cannot be copied |
| Copy assignment | `= delete` | Cannot be assigned |
| Move constructor | `noexcept = default` | Can be moved |
| Move assignment | `noexcept = default` | Can be moved |
| Destructor | Implicit | Does not call `DestroyWindow`; cleanup relies on process exit |

`Window_T` currently holds only the `HINSTANCE` and `HWND`:

```cpp
struct aru::window::Window_T {
    HINSTANCE instance = nullptr;
    HWND hwnd = nullptr;
};
```

## Unicode Titles

- C++ string literals are UTF-8 with Clang.
- `create_window()` converts the title with `MultiByteToWideChar(CP_UTF8, ...)` before passing it to `CreateWindowExW`.
- Do not pass ANSI/`...A` pointers to `...W` APIs. Doing so reinterprets bytes as UTF-16; for example, `"Arunimesta"` can appear as CJK characters (`U+7241 U+6E75 U+6D69 U+7365 U+6174`).

## Current Limitations

- No keyboard, mouse, or other input events are exposed.
- No resize handling (`WM_SIZE`).
- `DestroyWindow` is not called explicitly; the window is cleaned up when the process exits.
- The window class name is fixed to `ArunimestaClass`.
- The error type is `WindowError` and does not yet carry the Win32 error code.
- No thread-safety guarantees.

## Compatibility

- Windows x64 with Win32 (`user32`, `gdi32`).
- Requires C++23.
- Imported via `import aru;` or `import aru.window;`.
- No API stability guarantee yet; see [roadmap](../roadmap.md).
