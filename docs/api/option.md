# API: `aru::standard::Option<T>`

Reference for the `Option` class template exported by the `aru.standard:option` partition and re-exported by `aru`.

```cpp
import aru;                  // or: import aru.standard;
// or: import aru.standard:option;

aru::standard::Option<T>
```

## Overview

`Option<T>` holds **either** a value of type `T` (`Some`) **or nothing** (`None`). It is the engine's equivalent of Rust's `Option<T>` / an annotation-light alternative to `std::optional<T>`, built with the same union + active-flag pattern as [`Result<T, E>`](result.md).

- The value lives in a union and is constructed/destroyed explicitly with `std::construct_at` / `std::destroy_at`.
- No exceptions are thrown by the API. `unwrap()` calls `std::terminate()` when the option is empty.
- Move operations empty the source, matching the convention used by `String`, `Vector`, and the RHI resource wrappers.

## Creation

Creation goes through static factories; constructors are private.

```cpp
auto a = Option<int>::some(42);
auto b = Option<int>::none();
```

| Factory | Description |
| :--- | :--- |
| `static Option some(const T& value)` | Copies `value` into the option. |
| `static Option some(T&& value)` | Moves `value` into the option; `noexcept` when `T` is nothrow move-constructible. |
| `template<typename... Args> static Option someInPlace(Args&&... args)` | Constructs `T` directly in the union without an intermediate object. |
| `static Option none() noexcept` | Creates an empty option. |

```cpp
struct Point { int x; int y; };
auto point = Option<Point>::someInPlace(3, 4);
```

## Observers

| Function | Description |
| :--- | :--- |
| `bool isSome() const noexcept` | `true` when a value is stored. |
| `bool isNone() const noexcept` | `true` when empty. |
| `explicit operator bool() const noexcept` | Same as `isSome()`; usable in `if`. |

## Access

| Function | Description |
| :--- | :--- |
| `T& value() &` | Mutable reference to the stored value. |
| `const T& value() const&` | Const reference to the stored value. |
| `T&& value() &&` | Rvalue reference to the stored value. |
| `T unwrap()` | Moves the value out and empties the option. Terminates when empty. |
| `T unwrapOr(T fallback)` | Returns the stored value, or `fallback` when empty. |
| `template<typename Func> T valueOr(Func&& fn)` | Returns the stored value; invokes `fn()` on `None`. |

`value()` does not check `isSome()`; check first, or use `unwrap()` / `valueOr()` for consuming access. Like [`Result::valueOrUnexpected()`](result.md), `valueOr()` detects a `move()` member on `T` (as `String`, `Array`, `Vector`, and `Window` have) and uses it to return a prvalue.

```cpp
auto name = Option<aru::standard::String<char8_t>>::some(
    aru::standard::String<char8_t>::from("Arunimesta")
);

auto resolved = name.valueOr([] {
    return aru::standard::String<char8_t>::from("unnamed");
});
```

A `void` callback is treated as a terminal handler: after it runs, `std::terminate()` is called because no `T` can be returned.

## Transformations

These consume the option: the stored value is moved into the callback and the source becomes `None`.

| Function | Description |
| :--- | :--- |
| `map(Func&& fn)` | Applies `fn(T)` and wraps the result in `Option<U>`. Returns `None` when empty. |
| `andThen(Func&& fn)` | Like `map`, but `fn` must return an `Option<U>`; flattens one level. Returns `None` when empty. |
| `orElse(Func&& fn)` | Returns the current option when `Some`; otherwise calls `fn()` and returns its `Option<T>`. |

```cpp
auto number = Option<int>::some(42);

auto doubled = number.map([](int v) { return v * 2; });              // Some(84)
auto next = doubled.andThen([](int v) { return Option<int>::some(v + 1); }); // Some(85)

auto missing = Option<int>::none();
auto fallback = missing.orElse([] { return Option<int>::some(7); }); // Some(7)
```

`andThen` is constrained by `aru::standard::isOptionV`, so a callback returning a plain value is rejected at compile time; use `map` for that.

## Other Operations

| Function | Description |
| :--- | :--- |
| `void reset() noexcept` | Destroys the stored value and becomes `None`. |
| `Option take()` | Moves the value into a new option; source becomes `None`. |
| `Option copy() const` | Deep-copies the stored value (or returns `None`). |
| `Option move()` | Moves the value out; source becomes `None`. `noexcept` when `T` is nothrow move-constructible. |

## Rule of Five

| Operation | Behavior |
| :--- | :--- |
| Copy constructor / copy assignment / `copy()` | Constructs a fresh `T` from the source value; both options remain valid and independent. |
| Move constructor / move assignment / `move()` | Moves the value into the destination and resets the source. `noexcept` when `T` is nothrow move-constructible. |
| Destructor | Calls `std::destroy_at` on the active value only. |

```text
~Option()
  └─ active ? ~T() : nothing
```

## Example

```cpp
import aru;

using aru::standard::Option;

auto find(int id) -> Option<int> {
    if (id == 1) {
        return Option<int>::some(100);
    }
    return Option<int>::none();
}

int main() {
    auto found = find(1)
        .map([](int v) { return v * 2; })
        .valueOr([] { return -1; });

    aru::standard::println("result: {}", found); // 200
}
```

## Current Limitations

- No `constexpr` support; the union lifetime management is runtime-only.
- No `std::formatter` specialization; print the contained value via `value()`.
- No comparison operators and no `none_t` sentinel value; use `isNone()`.
- `map` / `andThen` invoke callbacks with `T` (by value), so they consume the option.
- Not a drop-in replacement for `std::optional<T>`; the API follows Rust-style names in `camelCase`.

## Compatibility

- Requires C++23 (`std::invocable`, named modules).
- Imported via `import aru;`, `import aru.standard;`, or `import aru.standard:option;`.
- No API stability guarantee yet; see [roadmap](../roadmap.md).
