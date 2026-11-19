# API: `aru::standard::Array<T>`

Reference for the `Array` class template exported by the `aru.standard:array` partition and re-exported by `aru.standard` / `aru`.

```cpp
import aru;                  // or: import aru.standard;
// or: import aru.standard:array;

aru::standard::Array<T>
```

## Overview

`Array<T>` is a **fixed-size** sequence. Its length is set at creation and never changes; unlike `Vector<T>`, it has no `push`/`pop`/`reserve`. Storage is raw memory from `std::allocator<T>`, with elements constructed via `std::construct_at` and destroyed explicitly.

Allocation follows the same policy as `String` and `Vector`:

| Task | How |
| :--- | :--- |
| New zero-initialized array | `Array<T>::create(size)` |
| From a literal list | `Array<T>::create({1, 2, 3})` or `Array<int> a = {1, 2, 3};` |
| Copy of another array | `b = a;` or `b = a.copy()` |
| Move from another array | `b = std::move(a);` or `b = a.move()` |
| Assign a literal list | `a = {1, 2, 3};` |

Rule of Five: copy/move **constructors are implemented** (deep copy / steal); the destructor is public.

## API

### `static Array create(std::size_t size)`

Allocates `size` value-initialized elements (`T{}`). `create(0)` is valid and produces an empty array with `begin() == end()`.

### `static Array create(std::initializer_list<T> values)`

Allocates an array containing exactly the literal list. Requires copy-constructible `T` because `initializer_list` elements are `const`.

### `Array(std::initializer_list<T> values)`

Literal constructor, equivalent to `create({...})`.

### `Array copy() const`

Deep copy: a new buffer with copies of all elements. Source is unchanged.

### `Array move() noexcept`

Transfers the buffer and length to the returned prvalue; the source becomes empty.

### `Array& operator=(const Array& source)` and `Array& operator=(Array&& source) noexcept`

Copy assignment deep-copies the elements; move assignment steals the buffer and empties the source. Self-assignment is a no-op.

### `Array& operator=(std::initializer_list<T> values)`

Replaces the contents with a deep copy of the literal list. The new length is `values.size()`.

### Element access and iteration

```cpp
T& operator[](std::size_t index) noexcept;
const T& operator[](std::size_t index) const noexcept;
std::size_t length() const noexcept;
bool empty() const noexcept;
T* begin() noexcept;
const T* begin() const noexcept;
T* end() noexcept;
const T* end() const noexcept;
```

`operator[]` does not perform bounds checks. `begin()`/`end()` are raw pointers, so range-for works directly:

```cpp
for (const T& value : array) {
    // ...
}
```

## Lifetime and Ownership

| Operation | Status | Notes |
| :--- | :--- | :--- |
| Default constructor | Not exposed | Use `create(...)` |
| Copy constructor | Public | Deep copy (`Array b = a;`), same as `b = a;` / `b = a.copy()` |
| Move constructor | `noexcept` | Steals the buffer and empties `a`; same as `b = std::move(a);` / `b = a.move()` |
| Destructor | Public | Destroys elements and deallocates only when `data != nullptr` |

If copying an element throws, already-constructed elements are destroyed and the raw buffer is freed before the exception propagates.

## Current Limitations

- Fixed length; no insertion, removal, or resize.
- No bounds-checked `at()`.
- No `std::formatter` / `std::hash` support yet.
- No allocator customization.

## Compatibility

- C++23 named modules.
- Requires copy-constructible `T` for `create({...})`, literal construction, and `copy()`.
- `create(size)` additionally requires `T` to be value-initializable.
