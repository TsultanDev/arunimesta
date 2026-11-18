# API: `aru::standard::Vector<T>`

Reference for the `Vector` class template exported by the `aru.standard:vector` partition and re-exported by `aru`.

```cpp
import aru;                    // or: import aru.standard;
// or: import aru.standard:vector;

aru::standard::Vector<T>
```

## Overview

`Vector<T>` is a growable dynamic array of `T`. Storage is raw memory allocated with `std::allocator<T>`; elements are constructed with placement-new and destroyed explicitly, so `T` does not need to be default-constructible.

**Allocation is factory-only.** The constructors are hidden or deleted; objects are created through static/member functions:

| Task | How |
| :--- | :--- |
| New allocation | `Vector<T>::create(std::size_t capacity)` |
| Literal list construction | `Vector<int> t = {1, 2, 3};` (requires copy-constructible `T`) |
| Copy of another vector | `b = a;` (copy assignment) or `b = a.copy()` |
| Move from another vector | `b = std::move(a);` (move assignment) or `b = a.move()` |
| Assign a literal list | `v = {1, 2, 3};` (initializer-list assignment) |

Copy and move **constructors are deleted**: `Vector b = a;` and `Vector b = std::move(a);` do not compile. The default constructor and the capacity constructor are private. The destructor stays public (stack objects must be destructible).

## API

### `static Vector create(std::size_t capacity)`

Allocates a new vector with room for `capacity` elements (zero elements initially). `create(0)` is valid.

```cpp
auto v = aru::standard::Vector<int>::create(4);
```

### `Vector copy() const`

Returns a new vector containing a deep copy of the elements; the source is unchanged.

```cpp
auto b = a.copy();
```

### `Vector move() noexcept`

Moves the contents out; the source becomes empty.

```cpp
auto b = a.move();   // a is empty afterwards
```

### `operator=(const Vector&)` / `operator=(Vector&&) noexcept` / `operator=(std::initializer_list<T>)`

Copy assignment clones the elements (self-assignment is safe). Move assignment steals the buffer and empties the source. Initializer-list assignment replaces the contents:

```cpp
v = {1, 2, 3};
```

### `void push(const T&)` / `void push(T&&)`

Appends an element. When `length() == capacity()`, the capacity is grown automatically (4 → 8 → 16 → ...); existing elements are relocated with `std::move_if_noexcept`.

### `void pop() noexcept`

Removes the last element. No-op on an empty vector; read the last element with `operator[]` beforehand.

### `T& operator[](std::size_t)` / `const T& operator[](std::size_t) const`

Element access, both read (`x = a[1]`) and write (`a[1] = x`). Valid indices are `[0, length()`; out-of-bounds access is undefined behavior (no bounds checking).

### `std::size_t length() / capacity()` and `bool empty()`

Number of active elements, allocated capacity, and emptiness check.

### `void reserve(std::size_t newCapacity)`

Pre-grows the capacity; does nothing when `newCapacity <= capacity()`.

### `VectorIterator<T> iter() noexcept` / `VectorIterator<const T> iter() const noexcept`

Returns a non-owning iterator over the elements. The iterator has `operator*`, prefix/postfix `operator++`, and `operator==`; member `begin()/end()` make range-based `for` work. `iter()` on a const vector yields read-only elements.

```cpp
for (int& value : v.iter()) { value += 1; }
for (const int& value : cv.iter()) { sum += value; }
```

### `VectorIntoIter<T> intoIter() noexcept`

Consuming iterator: takes ownership of the buffer and empties the vector. `operator*` returns `T&&` (elements are moved out), `done()` reports exhaustion, and `operator++` advances. Remaining elements (including skipped ones) are destroyed when the iterator is exhausted or destroyed.

```cpp
auto consume = v.intoIter();   // v is now empty
while (!consume.done()) {
    int value = *consume;
    ++consume;
}
```

### `value_type`

`using value_type = T;` is provided for generic code.

### Constructors and literal lists

| Operation | Status | Notes |
| :--- | :--- | :--- |
| `Vector(std::size_t capacity)` | Private | Hidden; use `create()`. |
| `Vector(std::initializer_list<T>)` | Public | `Vector<int> t = {1, 2, 3};` — needs copy-constructible `T` (`initializer_list` elements are `const`). |
| Copy constructor | Deleted | Use `b = a;` or `b = a.copy()`. |
| Move constructor | Deleted | Use `b = std::move(a);` or `b = a.move()`. |
| Copy/move assignment | Public | The only direct copy/move paths. |
| Default constructor | Private | Hidden; use `create()`. |
| Destructor | Public | Must stay public: stack objects must be destructible. |

### `Vector(std::initializer_list<T>)`

Constructs a vector directly from a literal list: `Vector<int> t = {1, 2, 3};`. It allocates exactly `list.size()` and copy-constructs each element (guaranteed copy elision applies, so no copy/move constructor of `Vector` itself is involved).

Limitations inherited from `std::initializer_list` (its elements are always `const`):

- `T` must be copy-constructible. Non-copyable element types (for example a nested `aru` `Vector`, or `aru::standard::String`) cannot be list-constructed; build them with `create()` + `push()`/assignment instead.
- `push()` likewise requires a copy- or move-constructible `T`.

## Current Limitations

- No bounds checking on `operator[]`.
- No `insert`, `erase`, `clear`, or `remove` yet.
- No `std::formatter` / `std::hash` support yet.
- Relocation provides the basic exception guarantee.

## Compatibility

- Requires C++23 (`std::allocator`, `std::destroy_n`, named modules).
- Imported via `import aru;`, `import aru.standard;`, or `import aru.standard:vector;`.
- No API stability guarantee yet; see [roadmap](../roadmap.md).
