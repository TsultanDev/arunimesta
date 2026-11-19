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
Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>);
```

Constructs a result holding either the value or the error. Construction from a `T` marks the result as "ok"; construction from an `E` marks it as "error".

Two in-place tags are also available to construct `T` / `E` directly in the union without an intermediate object:

```cpp
Result(std::in_place, Args&&... args);                 // construct T in the union
Result(aru::standard::errorInPlace, Args&&... args);   // construct E in the union
```

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

### `template<typename Func> requires std::invocable<Func&&, E&&> T valueOrUnexpected(Func&& fn)`

Returns the stored value when the result is ok. If `T` has a `move()` member (as `String`, `Array`, `Vector`, and `Window` do), it is used to return a prvalue; otherwise `std::move` is used.

On error, the callback is invoked with `E&&` (so lambdas taking either `E&&` or `const E&` work). If the callback returns a `T`, that value is returned:

```cpp
auto value = result.valueOrUnexpected([](std::string&& err) {
    std::println(stderr, "failed: {}", err);
    return -1;
});
```

If the callback returns `void`, it is treated as a terminal error handler: after it runs, `std::terminate()` is called (no `T` exists to return). This is the form used in `main.cpp`:

```cpp
auto window = Window::create("Arunimesta", 800, 600)
    .valueOrUnexpected([](aru::window::Error&& err) {
        aru::standard::println("Error! : {}", err);
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

- `Result` implements Rule of Five: copy/move constructors and assignments operate on the active union member. The move operations are `noexcept` only when both `T` and `E` are nothrow move-constructible.
- No `value()` / `error()` accessors yet; use `valueOrUnexpected()`.
- No monadic operations (`and_then`, `transform`, `or_else`).
- Not a drop-in replacement for `std::expected` in public APIs; `std::expected` remains the engine-wide convention for factories.

## Compatibility

- Requires C++23 (`std::invocable`, named modules).
- Imported via `import aru;`, `import aru.standard;`, or `import aru.standard:result;`.
- No API stability guarantee yet; see [roadmap](../roadmap.md).
