# API: `aru::standard::Array<T, N>`

Reference for the `Array` class template exported by the `aru.standard:array` partition and re-exported by `aru.standard` / `aru`.

```cpp
import aru;                  // or: import aru.standard;
// or: import aru.standard:array;

aru::standard::Array<T, N>
```

## Overview

`Array<T, N>` is a **fixed-size** sequence. `N` is a compile-time part of the type, and the elements live in an inline `T data[N]` member. There is no heap allocation, no allocator, and no manual `construct_at` / `destroy_n`. It behaves like a small, owning C array with a container-like interface.

`N` must be greater than zero (`static_assert`).

| Task | How |
| :--- | :--- |
| New zero-initialized array | `Array<T, N>::create()` |
| From a literal list | `Array<T, N>::create({1, 2, 3})` or `Array<int, 3> a = {1, 2, 3};` |
| Copy of another array | `b = a;` or `b = a.copy()` |
| Move from another array | `b = std::move(a);` or `b = a.move()` |
| Assign a literal list | `a = {1, 2, 3};` |

Rule of Five: copy/move constructors, assignments, and the destructor are `= default`. With inline storage there is no resource to steal: copying copies elements and moving transfers them element by element. The moved-from array remains valid (its elements are in a moved-from state).

## API

### `static Array create()`

Value-initializes all `N` elements with `T{}`.

### `static Array create(std::initializer_list<T> values)`

Equivalent to the literal constructor. `values.size()` may be smaller than `N`; the remaining elements are value-initialized. If it is larger than `N`, `std::length_error` is thrown.

```cpp
auto a = aru::standard::Array<int, 4>::create({1, 2}); // {1, 2, 0, 0}
```

### `Array(std::initializer_list<T> values)`

Literal constructor, equivalent to `create({...})`. It is not `explicit`, so `Array<int, 3> a = {7, 8, 9};` works.

### `Array copy() const` and `Array move()`

Return a new `Array` built from a copy / move of `*this`.

### `Array& operator=(const Array&)`, `Array& operator=(Array&&)`, `Array& operator=(std::initializer_list<T>)`

Defaulted copy/move assignment, plus list assignment. Self-assignment is safe. List assignment replaces all `N` elements, value-initializing any remainder.

### Element access and iteration

```cpp
T& operator[](std::size_t index) noexcept;
const T& operator[](std::size_t index) const noexcept;
std::size_t length() const noexcept;   // always N
bool empty() const noexcept;           // always false (N > 0)
T* begin() noexcept;
const T* begin() const noexcept;
T* end() noexcept;                     // data + N
const T* end() const noexcept;
```

`operator[]` does not perform bounds checks. `begin()` / `end()` are raw pointers into the inline storage, so range-for works directly:

```cpp
for (const T& value : array) {
    // ...
}
```

### `using value_type = T;` and `static constexpr std::size_t extent = N;`

Both are exposed for generic code.

## Lifetime and Ownership

| Operation | Status | Notes |
| :--- | :--- | :--- |
| Default constructor | Private | Use `create()` / the literal constructor |
| Copy constructor | `= default` | Copies all elements |
| Move constructor | `= default` | Moves all elements element-wise; source stays valid |
| Copy / move assignment | `= default` | Element-wise; self-assignment is safe |
| Destructor | `= default` | Destroys the inline array automatically |

## Current Limitations

- Fixed compile-time length; no insertion, removal, or resize.
- `N == 0` is not supported (`static_assert`).
- No bounds-checked `at()`.
- No `std::formatter` / `std::hash` support yet.
- No allocator customization.

## Compatibility

- C++23 named modules.
- Requires `T` to be default-constructible for `create()` and copy-constructible / assignable for the list constructor and `copy()`.
