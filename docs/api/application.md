# API: `aru::Application`

Reference for the `Application` class exported by module `aru`.

```cpp
import aru;

aru::Application
```

## Overview

`Application` represents the basic configuration of an application: its name and version. Objects are created through the `init()` factory and are *move-only*.

Implementation details are stored in `Application_T` (not exported) and owned through `std::unique_ptr`.

## API

### `static std::expected<Application, bool> init(std::string name, uint32_t version)`

Creates an `Application` instance.

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `name` | `std::string` | Application name. Stored by move. |
| `version` | `uint32_t` | Raw version value; there is no special encoding yet. |

Returns:

- A `std::expected` containing an `Application` on success.
- Currently the implementation **always returns success**; there is no failure path.
- The `bool` error type is a placeholder and does not provide information about the cause.

Side effects (debug builds only):

- Prints an application creation log to `stdout`.

Example:

```cpp
auto app = aru::Application::init("Arunimesta", 1);
if (!app) {
    return 1;
}
```

### `void printInfo() const`

Prints the application name and version to `stdout`.

It has no debug guard; calls still produce output in Release builds.

Example output:

```text
Aru: Application(created!!), name: Hello, version: 1
```

## Lifetime and Ownership

| Operation | Status | Notes |
| :--- | :--- | :--- |
| Constructor | Private | Use `init()` |
| Copy constructor | `= delete` | Cannot be copied |
| Copy assignment | `= delete` | Cannot be assigned |
| Move constructor | `noexcept = default` | Can be moved |
| Move assignment | `noexcept = default` | Can be moved |
| `operator new` | `= delete` | Cannot `new Application(...)` |
| Destructor | Implicit | `Application_T` is destroyed via `unique_ptr` |

Destruction order:

```text
~Application()
  └─ ~unique_ptr<Application_T>()
       └─ ~Application_T()
```

## Usage Examples

### `transform` style (as in `main.cpp`)

```cpp
#include <expected>
#include <utility>
import aru;

int main() {
    auto result = aru::Application::init("Hello", 1)
        .transform([](aru::Application&& app) -> void {
            app.printInfo();
        });

    return 0;
}
```

In this style, failures are not handled explicitly. It is suitable for examples, but not recommended for production code.

### Explicit check style

```cpp
import aru;

int main() {
    auto app = aru::Application::init("Arunimesta", 1);
    if (!app) {
        return 1;
    }

    app->printInfo();
    return 0;
}
```

### Moving an object

```cpp
#include <utility>
import aru;

int main() {
    auto app = aru::Application::init("Arunimesta", 1);
    if (!app) {
        return 1;
    }

    aru::Application moved = std::move(*app);
    moved.printInfo();
    return 0;
}
```

## Current Limitations

- There is no `Application::run()` or bootstrap callback yet.
- `version` is still a raw number; there is no `major.minor.patch` encoding helper.
- The `bool` error type is not descriptive.
- `printInfo()` is not wrapped in `#ifndef NDEBUG`, so it always prints.
- There are no thread-safety guarantees.

For windowing, see [`window.md`](window.md).

## Compatibility

- Requires C++23 (`std::expected`, `std::print`, named modules).
- Imported via `import aru;`.
- There is no API stability guarantee yet; see [roadmap](../roadmap.md).
