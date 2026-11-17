# API: `aru::standard::Result<T, E>`

Reference for the `Result` class template exported by the `aru.standard:result` partition and re-exported by `aru`.

```cpp
import aru;                  // or: import aru.standard;
// or: import aru.standard:result;

aru::standard::Result<T, E>
```

## Overview

`Result<T, E>` holds **exactly one** of a success value `T` or an error value `E` in a union, plus a `has_value` flag. It is a lightweight alternative to `std::expected` / `std::variant` for functions that return one of two values.

- `T` and `E` do not need to share a base type.
- Destruction goes through `std::destroy_at` on whichever member is active.
- No exceptions are thrown.

## API

### Constructors

```cpp
Result(const T& value);
Result(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>);
Result(const E& error);
Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<T>);
```

Constructs a result holding either the value or the error. Construction from a `T` marks the result as "ok"; construction from an `E` marks it as "error".

### `bool isOkOrErr() const noexcept`

`true` when the result holds a value, `false` when it holds an error.

### `explicit operator bool() const noexcept`

Same as `isOkOrErr()`; usable in conditions:

```cpp
aru::standard::Result<int, std::string> r = 42;
if (r) {
    // ok
}
```

### `template<std::invocable<const E&> Func> T valueOrUnexpected(Func&& fn)`

Returns the stored value with move semantics when the result is ok. Otherwise, invokes `fn` with the error and returns its result:

```cpp
auto value = result.valueOrUnexpected([](const std::string& err) {
    std::println(stderr, "failed: {}", err);
    return -1;
});
```

## Example

```cpp
import aru;

aru::standard::Result<int, std::string> parse(bool ok) {
    if (ok) {
        return aru::standard::Result<int, std::string>(42);
    }
    return aru::standard::Result<int, std::string>(std::string("bad input"));
}
```

## Lifetime and Ownership

| Operation | Status | Notes |
| :--- | :--- | :--- |
| Copy / move of `Result` | Not defined yet | Only construction from `T` / `E` is provided. |
| Destructor | Defined | `std::destroy_at` on the active union member. |

Destruction order:

```text
~Result()
  └─ has_value ? ~T() : ~E()
```

## Current Limitations

- No copy/move constructors or assignment operators for `Result` itself yet.
- No `value()` / `error()` accessors yet; use `valueOrUnexpected()`.
- No monadic operations (`and_then`, `transform`, `or_else`).
- Not a drop-in replacement for `std::expected` in public APIs; `std::expected` remains the engine-wide convention for factories.

## Compatibility

- Requires C++23 (`std::invocable`, named modules).
- Imported via `import aru;`, `import aru.standard;`, or `import aru.standard:result;`.
- No API stability guarantee yet; see [roadmap](../roadmap.md).
