# Code Conventions

This document summarizes the conventions used (and targeted) by the Arunimesta code. There is no `.clang-format` file in the repository yet, so consistency is maintained manually.

## Naming

| Element | Convention | Example |
| :--- | :--- | :--- |
| Public class / struct | `PascalCase` | `Application`, `Window`, `Instance` |
| Internal implementation struct | `PascalCase` + `_T` | `Application_T` |
| Function / method | `camelCase` | `init`, `printInfo`, `create`, `createFromWindow` |
| Variable / parameter / member | `camelCase` | `app`, `name`, `version`, `char_size` |
| Enum type | `PascalCase` | `Flags`, `ApplicationError` |
| Enum value | `PascalCase` | `None`, `Resizable` |
| Concept | `lowercase` | *(none in the engine yet)* |
| Namespace | `lowercase` | `aru`, `winaru`, `renaru` |
| Module | `lowercase.dot` | `aru`, `winaru`, `winaru:window` |
| Module file | `lowercase.cppm` | `application.cppm` |
| Macro | `UPPER_SNAKE_CASE` | *(none in the engine yet)* |

Notes:

- Members are accessed explicitly with `this->` in class implementations.
- Parameters that store a value into a member may use the same name, for example `version(version)`.

## File Extensions and Locations

- C++ modules use the `.cppm` extension.
- The entry point uses `.cpp`.
- Each module lives in a top-level `<module_name>/src/`.
- Non-module headers/sources (if added later) go in a top-level `<module_name>/include/`.

## `.cppm` File Layout

```cpp
module;                              // global module fragment

#include <cstdint>                   // standard library / external library includes
#include <memory>

export module aru:widget;            // module / partition declaration

export namespace aru {
    class Widget {                   // public class, defined directly
        public:
            explicit Widget(std::uint32_t id) noexcept : mId(id) {}
            std::uint32_t id() const noexcept { return mId; }

        private:
            std::uint32_t mId;
    };
}
```

Rules:

1. Includes **must** be in the global module fragment (`module;` through `export module ...;`).
2. Do not put `#include` after `export module`, because it will be exposed to consumers.
3. Define member and free functions directly where they are declared. Do not split a declaration from its definition unless the member belongs to a class from another partition and needs a complete type from this partition.
4. Use a forward declaration only for a type owned by another partition when importing its full definition would create a cycle.
5. Declare everything consumers need with `export`.

## Facade and Partitions

For a module that is split up:

```cpp
// application.cppm
export module aru:application;
```

```cpp
// lib.cppm
export module aru;
export import :application;
```

- Consumers only need `import aru;`.
- Internal partitions are not imported directly by outside code.
- Partition names follow the file name without the extension.

## Templates and Helpers

- Prefer the C++ standard library over custom wrappers: `std::vector<T>`, `std::array<T, N>`, `std::string` and its UTF variants, `std::string_view`, `std::expected<T, E>`, and `std::optional<T>`.
- Keep internal helpers in an unexported `<namespace>::detail` namespace. Helpers and forward-declared helper functions may be marked `static` or `noexcept` as appropriate.
- Concepts must be defined in their first declaration; do not split a `concept` into a forward declaration plus definition (that pattern is only valid for classes).
- Comments inside modules follow the engine language (Indonesian) but should stay short and explanatory.

## Factory + Pimpl Pattern

Use the following pattern for classes that hold a resource/internal state:

```cpp
namespace aru {
    struct Thing_T;
    export class Thing;
}

struct aru::Thing_T {
    // internal state
};

class aru::Thing {
    public:
        static std::expected<Thing, ThingError> init(/* parameters */);
        // other public methods

        Thing(const Thing&) = delete;
        Thing& operator=(const Thing&) = delete;
        Thing(Thing&&) noexcept = default;
        Thing& operator=(Thing&&) noexcept = default;

        void* operator new(size_t) = delete;
        void* operator new[](size_t) = delete;

    private:
        Thing(std::unique_ptr<Thing_T> impl);
        std::unique_ptr<Thing_T> impl;
};
```

Requirements:

- Private constructor; creation goes through a static factory.
- The factory returns `std::expected<T, Error>`.
- Copy is deleted; move is defaulted.
- `operator new` is deleted so the object cannot be freely allocated on the heap.
- Internal implementation is not exported.

> In the current code, `Application` only deletes the scalar `operator new`. For consistency, also add `operator new[]` when cleaning up the API.

## Error Handling

- Use `std::expected<T, E>` with `std::unexpected(...)` for recoverable failures, with a meaningful error type (`ApplicationError`, `winaru::Error`, `renaru::Error`).
- Use `std::optional<T>` when a value may be absent; prefer the standard library over custom result/option wrappers.
- Avoid `throw` at public API boundaries.
- Do not ignore an `expected` result; check it or propagate it with `transform` / `and_then`.

Example:

```cpp
auto app = aru::Application::init("Arunimesta", 1);
if (!app) {
    // handle failure
    return 1;
}
app->printInfo();
```

## Logging

- Use `std::println` / `std::print`.
- Wrap it with `#ifndef NDEBUG` so it is only active in debug builds.
- Prefix format: `Aru: <Area>(<event>)`.

Example:

```cpp
#ifndef NDEBUG
std::println(stdout, "Aru: Window(created!!), title: {}", title);
#endif
```

## Windows Interop

- Include `<Windows.h>` in the global module fragment.
- Use the explicit Unicode (`...W`) Win32 APIs: `RegisterClassExW`, `CreateWindowExW`, `DefWindowProcW`, `PeekMessageW`, `DispatchMessageW`.
- Never mix ANSI (`...A`) and Unicode (`...W`) APIs for the same window. Mixing them corrupts text; for example, a title created with `CreateWindowExA` and handled by `DefWindowProcW` is re-read as UTF-16 and can appear as CJK characters.
- Narrow public APIs take UTF-8 text (`const char*`, `std::string`, or `std::string_view`). Convert to UTF-16 with `MultiByteToWideChar(CP_UTF8, ...)` before calling a `...W` function; `Window` also accepts a `const char16_t*` title and returns `const std::wstring&` from `windowTitle()`.
- Define `UNICODE` and `_UNICODE` for targets that include `<Windows.h>` (see `winaru/CMakeLists.txt`).

## Formatting

There is no automatic formatter yet. Follow the existing style:

- 4-space indentation.
- Allman braces on main class/function declarations (new line).
- One statement per line.
- Avoid overly long lines; wrap parameters or chained calls.
- Use `std::uint32_t`/`uint32_t` consistently per file (currently `uint32_t` with `<cstdint>`).

## New Module Checklist

- [ ] `.cppm` file with a clear `export module`.
- [ ] Includes in the global module fragment.
- [ ] Internal types named `<Type>_T` and not exported.
- [ ] Factory returns `std::expected`.
- [ ] Copy deleted, move defaulted.
- [ ] `operator new` deleted if objects may only be created through the factory.
- [ ] CMake target with `FILE_SET CXX_MODULES`.
- [ ] Added to the root `CMakeLists.txt`.
- [ ] Facade `export import` updated.
- [ ] Win32 code uses only the Unicode (`...W`) APIs and converts UTF-8 to UTF-16.
- [ ] Documentation in `docs/` updated.
