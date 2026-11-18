# API: `aru::window::Window`

Reference for the `Window` class exported by module `aru.window`.

```cpp
import aru;          // re-exports aru.window
// or: import aru.window;
// or: import aru.window:window;
```

## Overview

`Window` represents a Win32 window. Instances are created through the `create()` factories, which return `aru::standard::Result<Window, Error>`. The title is stored as `aru::standard::String<char16_t>` and passed directly to the Unicode Win32 APIs.

Copy and move **constructors are deleted**. Copying and moving are done through assignment or the `copy()` / `move()` member functions. Creating another real OS window is a separate operation: `createFromWindow()`.

## Error type

```cpp
enum class aru::window::Error {
    None = 0,
    ClassRegistrationFailed = 1,
    WindowCreationFailed = 2,
};
```

A `std::formatter<Error>` is exported, so `Error` can be printed directly:

```cpp
aru::standard::println("Error! : {}", err);
```

## API

### `static Result<Window, Error> create(const char* title, uint32_t width, uint32_t height)`

UTF-8 overload. The narrow title is converted to UTF-16 with `String<char8_t>` → `String<char16_t>` before being passed to `CreateWindowExW`.

```cpp
auto window = aru::window::Window::create("Arunimesta", 800, 600)
    .valueOrUnexpected([](aru::window::Error&& err) {
        aru::standard::println("Error! : {}", err);
    });
```

### `static Result<Window, Error> create(const char16_t* title, uint32_t width, uint32_t height)`

UTF-16 overload; the title is used as-is.

### `static Result<Window, Error> createFromWindow(const Window& source)`

Creates a **new OS window** using the source window's title, width, and height. This is different from copy assignment or `copy()`: it calls WinAPI again and produces a distinct `HWND`.

```cpp
auto second = aru::window::Window::createFromWindow(window)
    .valueOrUnexpected([](aru::window::Error&& err) {
        aru::standard::println("Error! : {}", err);
    });
// second.handle() != window.handle()
```

### `Window& operator=(const Window& source)` and `Window copy() const`

Copy assignment / `copy()` deep-copy the metadata and the title `String`. The source is unchanged. They deliberately do **not** copy the `HWND`: a copied object has the same configuration but no OS window. Use `createFromWindow()` when a real second window is needed.

### `Window& operator=(Window&& source) noexcept` and `Window move() noexcept`

Move assignment / `move()` steal the `HWND`, `HINSTANCE`, title, width, and height. The source window becomes empty (`handle() == nullptr`).

```cpp
auto moved = window.move();   // window is now empty
```

### `void event_loop() const`

Runs the blocking Win32 message loop (`GetMessageW` / `TranslateMessage` / `DispatchMessageW`). `WM_DESTROY` posts `WM_QUIT`, which ends the loop.

```cpp
window.event_loop();
```

### Accessors

| Function | Description |
| :--- | :--- |
| `HWND handle() const noexcept` | Native window handle, or `nullptr` when empty. |
| `const String<char16_t>& windowTitle() const noexcept` | UTF-16 title. |
| `uint32_t windowWidth() / windowHeight() const noexcept` | Configured size. |
| `bool empty() const noexcept` | `true` when `handle() == nullptr`. |

## Lifetime and Ownership

| Operation | Status | Notes |
| :--- | :--- | :--- |
| Constructor | Private | Use `create()` / `createFromWindow()` |
| Copy constructor | Deleted | Use `b = a;` or `b = a.copy()` |
| Move constructor | Deleted | Use `b = std::move(a);` or `b = a.move()` |
| Copy assignment | Public | Copies title/metadata; does not duplicate the OS window |
| Move assignment | `noexcept` | Steals the handle; source becomes empty |
| Destructor | Public | `DestroyWindow` is called only when `hwnd != nullptr` |

The title is a `String<char16_t>` value member. It is RAII: when it is non-empty, clearing/replacing it releases its internal buffer, and the `String` destructor frees the remaining storage. No manual `delete` on the title is performed.

The window class `ArunimestaWindowClass` is registered on first use. Registration is idempotent: `ERROR_CLASS_ALREADY_EXISTS` is ignored.

## Current Limitations

- No keyboard, mouse, resize, or other input events are exposed.
- `event_loop()` blocks until `WM_QUIT`; there is no polling variant.
- The window class name is fixed to `ArunimestaWindowClass`.
- `Error` does not yet carry the Win32 error code.
- No thread-safety guarantees.

## Compatibility

- Windows x64 with Win32 (`user32`, `gdi32`).
- Requires C++23 (named modules, `std::format`).
- Imported via `import aru;`, `import aru.window;`, or `import aru.window:window;`.
- No API stability guarantee yet; see [roadmap](../roadmap.md).
