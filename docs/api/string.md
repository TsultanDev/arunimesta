# API: `aru::standard::String`

Reference for the `String` class template exported by the `aru.standard:string` partition and re-exported by `aru`.

```cpp
import aru;                // or: import aru.standard;
// or: import aru.standard:string;

aru::standard::String<T>   // T is char8_t, char16_t, or char32_t
```

## Overview

`String<T>` is a UTF string type that supports the three Unicode code-unit types:

| `T` | Encoding | Typical literal |
| :--- | :--- | :--- |
| `char8_t` | UTF-8 | `u8"..."` |
| `char16_t` | UTF-16 | `u"..."` |
| `char32_t` | UTF-32 | `U"..."` |

Storage is a `std::vector<T>` that always ends with a null terminator, so `string_data()` can be passed to C-style APIs that expect a null-terminated string of that element type. A string is **copyable and movable**; it does not use the pimpl pattern.

The template is constrained by the exported concept `character`:

```cpp
template<typename T>
concept character = (
    std::same_as<T, char8_t> || std::same_as<T, char16_t> ||
    std::same_as<T, char32_t>
);
```

## API

### `static String from(const T* str)`

Creates a `String<T>` from a null-terminated C string of the same element type.

Example:

```cpp
auto s = aru::standard::String<char8_t>::from(u8"Hello");
```

### `template<character SourceT> static String from(const String<SourceT>& str)`

Creates a **new** `String<T>` from an existing string of any other (or the same) character type. The source string is not modified.

| Behavior | Description |
| :--- | :--- |
| `T == SourceT` | Direct copy of the storage, terminator included. |
| `T != SourceT` | Transcodes: decodes each code point from the source encoding, then re-encodes it into the target encoding (UTF-8 ⇄ UTF-16 ⇄ UTF-32). |

Invalid input sequences (broken UTF-8 lead/continuation bytes, overlong encodings, out-of-range code points, unpaired surrogates) decode to the replacement character `U+FFFD`; conversion never throws.

This is the intended conversion style: strings are **not converted in place**; a new string is created instead.

```cpp
auto u8s  = aru::standard::String<char8_t>::from(u8"halo 🌍 dunia");
auto u16s = aru::standard::String<char16_t>::from(u8s);   // UTF-8 -> UTF-16
auto u32s = aru::standard::String<char32_t>::from(u16s);  // UTF-16 -> UTF-32
auto u8b  = aru::standard::String<char8_t>::from(u32s);   // UTF-32 -> UTF-8
// u8b == u8s
```

### `const T* stringData() const noexcept` / `T* stringData() noexcept`

Pointer to the internal buffer. The buffer is always null-terminated (`T{}` terminator). A mutable pointer is returned from a non-const string; mutating it past `length()` is undefined behavior territory — avoid writing to the terminator.

### `std::uint32_t length() const noexcept`

Number of code units, excluding the terminator. For example, `u8"halo 🌍 dunia"` has length 15 in UTF-8, 13 in UTF-16 (the emoji is a surrogate pair), and 12 in UTF-32.

### `std::uint32_t byteSize() const noexcept`

`length() * sizeof(T)` — the number of bytes of code-unit storage, excluding the terminator.

### `bool empty() const noexcept`

`true` when the string has zero code units.

### `const T& operator[](std::uint32_t index) const noexcept` / `T& operator[](std::uint32_t index) noexcept`

Element access. Valid indices are `[0, length()` — reading `index == length()` returns the terminator; anything beyond is out of bounds.

### `begin() / end()`

Const iterators over the code units (the terminator is excluded by `end()`), so range-based `for` works:

```cpp
for (char32_t unit : u32s) {
    // ...
}
```

### `bool operator==(const String&) const noexcept`

Compares the raw storage; strings are equal when their code units (including the terminator) are identical.

### `String move() noexcept`

Moves the contents out; the source string becomes empty. The result type is **always the same type** as the source (`String<T>::move()` returns `String<T>`) — cross-type moves are not possible, only `UTF-8 → UTF-8`, `UTF-16 → UTF-16`, `UTF-32 → UTF-32`. Cross-type transfer goes through `from()` (which copies/transcodes).

```cpp
auto s8  = aru::standard::String<char8_t>::from(u8"halo");
auto s8b = s8.move();      // s8 is now empty, s8b owns the buffer
```

### Constructors and assignment

| Operation | Status | Notes |
| :--- | :--- | :--- |
| `String()` | Private | Hidden; allocation goes through `from()`. |
| `String(const T*)` | Private | Hidden; use `from(const T*)`. |
| Copy constructor | Public | Deep copy of the storage (same as `from()` with the same type or copy assignment). |
| Move constructor | `noexcept` | Steals the storage and empties the source; same as `move()` or move assignment. |
| Copy assignment | Public | Copies the storage. |
| Move assignment | `noexcept` | Moves the storage (`b = a.move()`). |
| `String(std::vector<T>)` | Private | Used internally by `from()` / `move()`; the vector must already end with a terminator. |
| Destructor | Public | Must stay public: stack objects must be destructible. `std::vector` cleans up. |

### `value_type`

`using value_type = T;` is provided for generic code.

## Internal Notes

- Different instantiations (`String<char8_t>`, `String<char16_t>`, ...) are unrelated classes; they are mutually friendly (`template<character U> friend class String;`) so `from()` can read the private storage of another instantiation.
- Encoding/decoding helpers live in the unexported `aru::standard::detail` namespace: `decodeUtf8`, `decodeUtf16`, `decodeUtf32`, `encodeCodepoint`, and `cstrLength`.
- The UTF-8 decoder rejects overlong encodings, code points above `U+10FFFF`, and surrogates (`U+D800–U+DFFF`).

## Current Limitations

- No concatenation yet (`append`, `+`, `+=`).
- No `std::formatter` or `std::hash` support yet.
- `length()`/`byte_size()` are `uint32_t`; strings longer than ~4 billion units are out of scope.
- No allocator customization.

## Compatibility

- Requires C++23 (`std::concepts`, named modules).
- Imported via `import aru;`, `import aru.standard;`, or `import aru.standard:string;`.
- No API stability guarantee yet; see [roadmap](../roadmap.md).
