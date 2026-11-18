module;

#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>

export module aru.standard:vector;

export namespace aru{
    namespace standard{
        template<typename E>
        class VectorIterator;

        template<typename T>
        class VectorIntoIter;

        struct IntoIterEnd {};

        template<typename T>
        VectorIntoIter<T> begin(VectorIntoIter<T>& iterator) noexcept;

        template<typename T>
        IntoIterEnd end(VectorIntoIter<T>&) noexcept;

        template<typename T>
        bool operator==(const VectorIntoIter<T>& iterator, IntoIterEnd) noexcept;

        template<typename T>
        bool operator!=(const VectorIntoIter<T>& iterator, IntoIterEnd sentinel) noexcept;

        template<typename T>
        class Vector;

        template<typename T, typename Source>
        concept vectorSource =
            std::ranges::forward_range<Source> &&
            (!std::same_as<std::remove_cvref_t<Source>, Vector<T>>) &&
            (!std::integral<std::remove_cvref_t<Source>>) &&
            std::convertible_to<std::ranges::range_reference_t<Source>, T>;
    }
}

template<typename E>
class aru::standard::VectorIterator{
    public:
        E& operator*() const noexcept;
        VectorIterator& operator++() noexcept;
        VectorIterator operator++(int) noexcept;
        bool operator==(const VectorIterator&) const noexcept;

        VectorIterator begin() const noexcept;
        VectorIterator end() const noexcept;

        VectorIterator() = default;

    private:
        template<typename U>
        friend class aru::standard::Vector;

        VectorIterator(E* ptr, E* stop) noexcept;

        E* ptr = nullptr;
        E* stop = nullptr;
};

template<typename T>
class aru::standard::VectorIntoIter{
    public:
        T&& operator*() const noexcept;
        VectorIntoIter& operator++() noexcept;
        bool operator==(const VectorIntoIter&) const noexcept;
        bool done() const noexcept;

        VectorIntoIter() = default;
        VectorIntoIter(VectorIntoIter&&) noexcept;
        VectorIntoIter& operator=(VectorIntoIter&&) noexcept;
        VectorIntoIter(const VectorIntoIter&) = delete;
        VectorIntoIter& operator=(const VectorIntoIter&) = delete;
        ~VectorIntoIter();

    private:
        friend class aru::standard::Vector<T>;

        VectorIntoIter(T* data, std::size_t capacity, std::size_t current, std::size_t endIndex) noexcept;
        void release() noexcept;

        T* data = nullptr;             // buffer milik iterator ini
        std::size_t cap = 0;           // kapasitas alokasi
        std::size_t current = 0;       // posisi traversal
        std::size_t endIndex = 0;      // jumlah elemen yang dibangun
};

// Alokasi hanya melalui factory: create() untuk alokasi baru.
// Copy melalui assignment (=) atau copy(); move melalui move assignment
// atau move(). Copy/move constructor sengaja dihapus.
template<typename T>
class aru::standard::Vector{
    public:
        using value_type = T;

        // Alokasi baru: capacity(), daftar literal, atau container/range lain.
        static Vector create(std::size_t capacity);
        static Vector create(std::initializer_list<T> values);

        template<typename Source>
            requires vectorSource<T, Source>
        static Vector create(const Source& source);

        Vector(std::initializer_list<T> values);

        Vector& operator=(const Vector&);
        Vector& operator=(Vector&&) noexcept;
        Vector& operator=(std::initializer_list<T> values);

        Vector copy() const;

        Vector move() noexcept;

        void push(const T& value);
        void push(T&& value);
        void pop() noexcept;

        T& operator[](std::size_t index) noexcept;
        const T& operator[](std::size_t index) const noexcept;

        std::size_t length() const noexcept;
        std::size_t capacity() const noexcept;
        bool empty() const noexcept;
        void reserve(std::size_t newCapacity);

        // Range-for langsung: for (auto v : vec) { ... }
        VectorIterator<T> begin() noexcept;
        VectorIterator<const T> begin() const noexcept;
        VectorIterator<T> end() noexcept;
        VectorIterator<const T> end() const noexcept;

        VectorIterator<T> iter() noexcept;
        VectorIterator<const T> iter() const noexcept;

        VectorIntoIter<T> intoIter() noexcept;

        ~Vector();

    private:
        struct CopyTag {};
        struct RangeTag {};

        Vector() = default;
        explicit Vector(std::size_t capacity);
        explicit Vector(const Vector& other, CopyTag cloneTag);
        explicit Vector(T* data, std::size_t count, std::size_t capacity) noexcept;

        template<typename Source>
            requires vectorSource<T, Source>
        explicit Vector(const Source& source, RangeTag);

        template<typename Source>
        Vector(std::size_t capacity, RangeTag, const Source& source);

        void cloneFrom(const Vector& other);
        void makeRoom();
        void relocate(std::size_t newCapacity);
        void destroyAll() noexcept;
        void swap(Vector& other) noexcept;

        T* data = nullptr;
        std::size_t count = 0;
        std::size_t cap = 0;
};

template<typename T>
aru::standard::Vector<T> aru::standard::Vector<T>::create(std::size_t capacity){
    return Vector<T>(capacity);
}

template<typename T>
aru::standard::Vector<T> aru::standard::Vector<T>::create(std::initializer_list<T> values){
    return Vector<T>(values);
}

template<typename T>
template<typename Source>
    requires aru::standard::vectorSource<T, Source>
aru::standard::Vector<T> aru::standard::Vector<T>::create(const Source& source){
    return Vector<T>(source, RangeTag{});
}

template<typename T>
aru::standard::Vector<T>::Vector(std::size_t capacity) :
    data(capacity > 0 ? std::allocator<T>{}.allocate(capacity) : nullptr),
    count(0),
    cap(capacity)
{}

template<typename T>
aru::standard::Vector<T>::Vector(std::initializer_list<T> values) :
    Vector(values, RangeTag{})
{}

// Rentang/container lain: alokasi tepat sejumlah elemen lalu salin.
template<typename T>
template<typename Source>
    requires aru::standard::vectorSource<T, Source>
aru::standard::Vector<T>::Vector(const Source& source, RangeTag) :
    Vector(static_cast<std::size_t>(std::ranges::distance(source)), RangeTag{}, source)
{}

template<typename T>
template<typename Source>
aru::standard::Vector<T>::Vector(std::size_t capacity, RangeTag, const Source& source) :
    data(capacity > 0 ? std::allocator<T>{}.allocate(capacity) : nullptr),
    count(0),
    cap(capacity)
{
    try {
        for (const auto& value : source) {
            ::new (static_cast<void*>(this->data + this->count)) T(value);
            ++this->count;
        }
    } catch (...) {
        std::destroy_n(this->data, this->count);
        std::allocator<T>{}.deallocate(this->data, this->cap);
        this->data = nullptr;
        this->count = 0;
        this->cap = 0;
        throw;
    }
}

template<typename T>
aru::standard::Vector<T>::Vector(const Vector& other, CopyTag) :
    data(other.count > 0 ? std::allocator<T>{}.allocate(other.count) : nullptr),
    count(0),
    cap(other.count)
{
    try {
        this->cloneFrom(other);
    } catch (...) {
        std::allocator<T>{}.deallocate(this->data, this->cap);
        this->data = nullptr;
        this->count = 0;
        this->cap = 0;
        throw;
    }
}

template<typename T>
aru::standard::Vector<T>::Vector(T* data, std::size_t count, std::size_t capacity) noexcept :
    data(data),
    count(count),
    cap(capacity)
{}

// Isi buffer (harus kosong) dengan salinan elemen other.
template<typename T>
void aru::standard::Vector<T>::cloneFrom(const Vector& other){
    std::size_t built = 0;
    try {
        for (; built < other.count; ++built) {
            ::new (static_cast<void*>(this->data + built)) T(other.data[built]);
        }
    } catch (...) {
        std::destroy_n(this->data, built);
        this->count = 0;
        throw;
    }
    this->count = other.count;
}

template<typename T>
aru::standard::Vector<T>& aru::standard::Vector<T>::operator=(const Vector& other){
    if (this == &other) {
        return *this;
    }

    if (this->data != nullptr) {
        std::destroy_n(this->data, this->count);
    }
    this->count = 0;

    if (other.count > this->cap) {
        if (this->data != nullptr) {
            std::allocator<T>{}.deallocate(this->data, this->cap);
            this->data = nullptr;
            this->cap = 0;
        }
        this->data = other.count > 0 ? std::allocator<T>{}.allocate(other.count) : nullptr;
        this->cap = other.count;
    }
    this->cloneFrom(other);
    return *this;
}

template<typename T>
aru::standard::Vector<T>& aru::standard::Vector<T>::operator=(Vector&& other) noexcept {
    if (this != &other) {
        this->destroyAll();
        this->data = std::exchange(other.data, nullptr);
        this->count = std::exchange(other.count, 0);
        this->cap = std::exchange(other.cap, 0);
    }
    return *this;
}

template<typename T>
aru::standard::Vector<T>& aru::standard::Vector<T>::operator=(std::initializer_list<T> values){
    if (this->data != nullptr) {
        std::destroy_n(this->data, this->count);
    }
    this->count = 0;

    if (values.size() > this->cap) {
        if (this->data != nullptr) {
            std::allocator<T>{}.deallocate(this->data, this->cap);
            this->data = nullptr;
            this->cap = 0;
        }
        this->data = values.size() > 0 ? std::allocator<T>{}.allocate(values.size()) : nullptr;
        this->cap = values.size();
    }

    for (const T& value : values) {
        ::new (static_cast<void*>(this->data + this->count)) T(value);
        ++this->count;
    }
    return *this;
}

template<typename T>
aru::standard::Vector<T> aru::standard::Vector<T>::copy() const{
    return Vector<T>(*this, CopyTag{});
}

template<typename T>
aru::standard::Vector<T> aru::standard::Vector<T>::move() noexcept{
    return Vector<T>(
        std::exchange(this->data, nullptr),
        std::exchange(this->count, std::size_t{0}),
        std::exchange(this->cap, std::size_t{0})
    );
}

template<typename T>
aru::standard::Vector<T>::~Vector<T>(){
    this->destroyAll();
}

template<typename T>
void aru::standard::Vector<T>::push(const T& value){
    this->makeRoom();
    ::new (static_cast<void*>(this->data + this->count)) T(value);
    ++this->count;
}

template<typename T>
void aru::standard::Vector<T>::push(T&& value){
    this->makeRoom();
    ::new (static_cast<void*>(this->data + this->count)) T(std::move(value));
    ++this->count;
}

template<typename T>
void aru::standard::Vector<T>::pop() noexcept{
    if (this->count == 0) {
        return;
    }
    --this->count;
    std::destroy_at(this->data + this->count);
}

template<typename T>
T& aru::standard::Vector<T>::operator[](std::size_t index) noexcept{
    return this->data[index];
}

template<typename T>
const T& aru::standard::Vector<T>::operator[](std::size_t index) const noexcept{
    return this->data[index];
}

template<typename T>
std::size_t aru::standard::Vector<T>::length() const noexcept{
    return this->count;
}

template<typename T>
std::size_t aru::standard::Vector<T>::capacity() const noexcept{
    return this->cap;
}

template<typename T>
bool aru::standard::Vector<T>::empty() const noexcept{
    return this->count == 0;
}

template<typename T>
void aru::standard::Vector<T>::reserve(std::size_t newCapacity){
    if (newCapacity > this->cap) {
        this->relocate(newCapacity);
    }
}

template<typename T>
aru::standard::VectorIterator<T> aru::standard::Vector<T>::begin() noexcept{
    return this->iter();
}

template<typename T>
aru::standard::VectorIterator<const T> aru::standard::Vector<T>::begin() const noexcept{
    return this->iter();
}

template<typename T>
aru::standard::VectorIterator<T> aru::standard::Vector<T>::end() noexcept{
    T* stop = this->data != nullptr ? this->data + this->count : nullptr;
    return VectorIterator<T>(stop, stop);
}

template<typename T>
aru::standard::VectorIterator<const T> aru::standard::Vector<T>::end() const noexcept{
    const T* stop = this->data != nullptr ? this->data + this->count : nullptr;
    return VectorIterator<const T>(stop, stop);
}

template<typename T>
aru::standard::VectorIterator<T> aru::standard::Vector<T>::iter() noexcept{
    T* stop = this->data != nullptr ? this->data + this->count : nullptr;
    return VectorIterator<T>(this->data, stop);
}

template<typename T>
aru::standard::VectorIterator<const T> aru::standard::Vector<T>::iter() const noexcept{
    const T* stop = this->data != nullptr ? this->data + this->count : nullptr;
    return VectorIterator<const T>(this->data, stop);
}

template<typename T>
aru::standard::VectorIntoIter<T> aru::standard::Vector<T>::intoIter() noexcept{
    return VectorIntoIter<T>(
        std::exchange(this->data, nullptr),
        std::exchange(this->cap, std::size_t{0}),
        0,
        std::exchange(this->count, std::size_t{0})
    );
}

// Pastikan ada slot bebas; grow ganda (4, 8, 16, ...) saat penuh.
template<typename T>
void aru::standard::Vector<T>::makeRoom(){
    if (this->count < this->cap) {
        return;
    }
    constexpr std::size_t maxCap = (std::numeric_limits<std::size_t>::max)();
    const std::size_t newCap = (this->cap == 0) ? 4 :
        ((this->cap > maxCap / 2) ? maxCap : this->cap * 2);
    this->relocate(newCap);
}

// Pindahkan semua elemen ke buffer baru. Elemen dipindah dengan
// move_if_noexcept agar buffer lama tetap valid jika relokasi gagal.
template<typename T>
void aru::standard::Vector<T>::relocate(std::size_t newCapacity){
    T* fresh = std::allocator<T>{}.allocate(newCapacity);
    std::size_t built = 0;
    try {
        for (; built < this->count; ++built) {
            ::new (static_cast<void*>(fresh + built)) T(std::move_if_noexcept(this->data[built]));
        }
    } catch (...) {
        std::destroy_n(fresh, built);
        std::allocator<T>{}.deallocate(fresh, newCapacity);
        throw;
    }

    if (this->data != nullptr) {
        std::destroy_n(this->data, this->count);
        std::allocator<T>{}.deallocate(this->data, this->cap);
    }
    this->data = fresh;
    this->cap = newCapacity;
}

template<typename T>
void aru::standard::Vector<T>::destroyAll() noexcept{
    if (this->data != nullptr) {
        std::destroy_n(this->data, this->count);
        std::allocator<T>{}.deallocate(this->data, this->cap);
    }
    this->data = nullptr;
    this->count = 0;
    this->cap = 0;
}

template<typename T>
void aru::standard::Vector<T>::swap(Vector& other) noexcept{
    std::swap(this->data, other.data);
    std::swap(this->count, other.count);
    std::swap(this->cap, other.cap);
}

template<typename E>
E& aru::standard::VectorIterator<E>::operator*() const noexcept{
    return *this->ptr;
}

template<typename E>
aru::standard::VectorIterator<E>& aru::standard::VectorIterator<E>::operator++() noexcept{
    ++this->ptr;
    return *this;
}

template<typename E>
aru::standard::VectorIterator<E> aru::standard::VectorIterator<E>::operator++(int) noexcept{
    VectorIterator<E> tmp = *this;
    ++this->ptr;
    return tmp;
}

template<typename E>
bool aru::standard::VectorIterator<E>::operator==(const VectorIterator& other) const noexcept{
    return this->ptr == other.ptr;
}

template<typename E>
aru::standard::VectorIterator<E> aru::standard::VectorIterator<E>::begin() const noexcept{
    return *this;
}

template<typename E>
aru::standard::VectorIterator<E> aru::standard::VectorIterator<E>::end() const noexcept{
    return VectorIterator<E>(this->stop, this->stop);
}

template<typename E>
aru::standard::VectorIterator<E>::VectorIterator(E* ptr, E* stop) noexcept :
    ptr(ptr),
    stop(stop)
{}

template<typename T>
T&& aru::standard::VectorIntoIter<T>::operator*() const noexcept{
    return std::move(this->data[this->current]);
}

template<typename T>
aru::standard::VectorIntoIter<T>& aru::standard::VectorIntoIter<T>::operator++() noexcept{
    if (this->data != nullptr) {
        ++this->current;
        if (this->current >= this->endIndex) {
            // Semua elemen sudah dilewati; bebaskan buffer dan
            // jadikan iterator ini setara dengan "end".
            this->release();
        }
    }
    return *this;
}

template<typename T>
bool aru::standard::VectorIntoIter<T>::operator==(const VectorIntoIter& other) const noexcept{
    return this->data == other.data && this->current == other.current;
}

template<typename T>
bool aru::standard::VectorIntoIter<T>::done() const noexcept{
    // Vector kosong yang punya kapasitas (data != nullptr, count == 0)
    // juga harus dianggap selesai.
    return this->data == nullptr || this->current >= this->endIndex;
}

template<typename T>
aru::standard::VectorIntoIter<T>::VectorIntoIter(VectorIntoIter&& other) noexcept :
    data(std::exchange(other.data, nullptr)),
    cap(std::exchange(other.cap, std::size_t{0})),
    current(std::exchange(other.current, std::size_t{0})),
    endIndex(std::exchange(other.endIndex, std::size_t{0}))
{}

template<typename T>
aru::standard::VectorIntoIter<T>& aru::standard::VectorIntoIter<T>::operator=(VectorIntoIter&& other) noexcept {
    if (this != &other) {
        this->release();
        this->data = std::exchange(other.data, nullptr);
        this->cap = std::exchange(other.cap, std::size_t{0});
        this->current = std::exchange(other.current, std::size_t{0});
        this->endIndex = std::exchange(other.endIndex, std::size_t{0});
    }
    return *this;
}

template<typename T>
aru::standard::VectorIntoIter<T>::~VectorIntoIter<T>(){
    this->release();
}

template<typename T>
aru::standard::VectorIntoIter<T>::VectorIntoIter(T* data, std::size_t capacity, std::size_t current, std::size_t endIndex) noexcept :
    data(data),
    cap(capacity),
    current(current),
    endIndex(endIndex)
{}

// Hancurkan elemen yang belum dikonsumsi dan bebaskan buffer.
// Elemen yang sudah di-move tetap harus dihancurkan sekali, jadi
// semua elemen [0, endIndex) dihancurkan di sini.
template<typename T>
void aru::standard::VectorIntoIter<T>::release() noexcept{
    if (this->data != nullptr) {
        std::destroy_n(this->data, this->endIndex);
        std::allocator<T>{}.deallocate(this->data, this->cap);
    }
    this->data = nullptr;
    this->cap = 0;
    this->current = 0;
    this->endIndex = 0;
}

// Range-for untuk IntoIter: __begin dipindah dari __range karena
// VectorIntoIter sengaja move-only (copy constructor dihapus).
template<typename T>
aru::standard::VectorIntoIter<T> aru::standard::begin(VectorIntoIter<T>& iterator) noexcept{
    return std::move(iterator);
}

template<typename T>
aru::standard::IntoIterEnd aru::standard::end(VectorIntoIter<T>&) noexcept{
    return IntoIterEnd{};
}

template<typename T>
bool aru::standard::operator==(const VectorIntoIter<T>& iterator, IntoIterEnd) noexcept{
    return iterator.done();
}

template<typename T>
bool aru::standard::operator!=(const VectorIntoIter<T>& iterator, IntoIterEnd sentinel) noexcept{
    return !(iterator == sentinel);
}
