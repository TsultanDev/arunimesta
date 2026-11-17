# API: `aru::standard::println` / `eprintln`

Reference for the printing helpers exported by the `aru.standard:print` partition and re-exported by `aru`.

```cpp
import aru;                 // or: import aru.standard;
// or: import aru.standard:print;

aru::standard::println(...)
aru::standard::eprintln(...)
```

## Overview

Thin wrappers around `std::println` with a consistent, no-`std::`-prefix entry point for engine code.

| Function | Output | Guard |
| :--- | :--- | :--- |
| `println(fmt, args...)` | `stdout` | Only in debug builds (`#ifndef NDEBUG`). |
| `println()` | `stdout` | Always active. |
| `eprintln(fmt, args...)` | `stderr` | Only in debug builds (`#ifndef NDEBUG`). |
| `eprintln()` | `stderr` | Always active. |

## API

### `template<typename... Args> void println(std::format_string<Args...> fmt, Args&&... args)`

Prints a formatted line to `stdout`, forwarding to `std::println(stdout, fmt, args...)`.

- Compiled and executed **only in debug builds** (`#ifndef NDEBUG`); in Release builds it is a no-op that still compiles.
- Format string follows `std::format` syntax.

### `void println()`

Prints an empty line to `stdout`. Always active (not debug-gated).

### `template<typename... Args> void eprintln(std::format_string<Args...> fmt, Args&&... args)`

Prints a formatted line to `stderr`, forwarding to `std::println(stderr, fmt, args...)`. Debug builds only; no-op in Release.

### `void eprintln()`

Prints an empty line to `stderr`. Always active.

## Example

```cpp
import aru;

aru::standard::println("Aru: Application(created!!), name: {}, version: {}", name, version);
aru::standard::eprintln("Aru: Window(created!!), title: {}", title);
aru::standard::println();   // empty line, always printed
```

## Logging Convention

Engine log lines use the `Aru: <Area>(<event>)` prefix, for example:

```text
Aru: Application(created!!)
Aru: Window(created!!), title: Arunimesta
```

## Current Limitations

- The formatted overloads are debug-only; Release builds silently discard them. Use `std::println` directly for output that must appear in Release.
- No `print` (without newline) wrappers yet.
- No log levels, timestamps, or sink customization.

## Compatibility

- Requires C++23 (`std::print` / `std::println`, named modules).
- Imported via `import aru;`, `import aru.standard;`, or `import aru.standard:print;`.
- No API stability guarantee yet; see [roadmap](../roadmap.md).
