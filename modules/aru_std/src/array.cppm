module;

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <utility>

export module aru.standard:array;

export namespace aru::standard{
    template<typename T>
    class Array;
}

// Array ukuran tetap. Alokasi baru lewat factory; Rule of Five lengkap
// (copy/move constructor tersedia), copy()/move() tetap disediakan.
template<typename T>
class aru::standard::Array{
    public:
        using value_type = T;

        static Array create(std::size_t size);
        static Array create(std::initializer_list<T> values);

        // literal constructor dipertahankan seperti Vector.
        Array(std::initializer_list<T> values);

        // Rule of Five: deep copy / pindahkan buffer.
        Array(const Array& other);
        Array(Array&& other) noexcept;
        Array& operator=(const Array& source);
        Array& operator=(Array&& source) noexcept;
        Array& operator=(std::initializer_list<T> values);

        Array copy() const;
        Array move() noexcept;

        T& operator[](std::size_t index) noexcept;
        const T& operator[](std::size_t index) const noexcept;

        std::size_t length() const noexcept;
        bool empty() const noexcept;

        T* begin() noexcept;
        const T* begin() const noexcept;
        T* end() noexcept;
        const T* end() const noexcept;

        ~Array();

    private:
        struct CopyTag {};
        struct MoveTag {};
        struct ValueInitTag {};

        explicit Array(std::size_t size, ValueInitTag tag);
        Array(const Array& source, CopyTag tag);
        Array(Array&& source, MoveTag tag) noexcept;

        void initFrom(const T* source, std::size_t count);
        void initValue(std::size_t count);
        void release() noexcept;

        T* data = nullptr;
        std::size_t count = 0;
};

template<typename T>
aru::standard::Array<T> aru::standard::Array<T>::create(std::size_t size){
    return Array<T>(size, ValueInitTag{});
}

template<typename T>
aru::standard::Array<T> aru::standard::Array<T>::create(std::initializer_list<T> values){
    return Array<T>(values);
}

template<typename T>
aru::standard::Array<T>::Array(std::initializer_list<T> values){
    this->initFrom(values.begin(), values.size());
}

template<typename T>
aru::standard::Array<T>::Array(const Array& other) :
    Array(other, CopyTag{})
{}

template<typename T>
aru::standard::Array<T>::Array(Array&& other) noexcept :
    data(std::exchange(other.data, nullptr)),
    count(std::exchange(other.count, std::size_t{0}))
{}

template<typename T>
aru::standard::Array<T>::Array(std::size_t size, ValueInitTag){
    this->initValue(size);
}

template<typename T>
aru::standard::Array<T>::Array(const Array& source, CopyTag){
    this->initFrom(source.data, source.count);
}

template<typename T>
aru::standard::Array<T>::Array(Array&& source, MoveTag) noexcept :
    data(std::exchange(source.data, nullptr)),
    count(std::exchange(source.count, std::size_t{0}))
{}

template<typename T>
void aru::standard::Array<T>::initFrom(const T* source, std::size_t size){
    if (size == 0){
        return;
    }

    std::allocator<T> allocator{};
    T* fresh = allocator.allocate(size);
    std::size_t built = 0;

    try{
        for (; built < size; ++built){
            std::construct_at(fresh + built, source[built]);
        }
    }catch(...){
        std::destroy_n(fresh, built);
        allocator.deallocate(fresh, size);
        throw;
    }

    this->data = fresh;
    this->count = size;
}

template<typename T>
void aru::standard::Array<T>::initValue(std::size_t size){
    if (size == 0){
        return;
    }

    std::allocator<T> allocator{};
    T* fresh = allocator.allocate(size);
    std::size_t built = 0;

    try{
        for (; built < size; ++built){
            std::construct_at(fresh + built);
        }
    }catch(...){
        std::destroy_n(fresh, built);
        allocator.deallocate(fresh, size);
        throw;
    }

    this->data = fresh;
    this->count = size;
}

template<typename T>
aru::standard::Array<T>& aru::standard::Array<T>::operator=(const Array& source){
    if (this != &source){
        Array fresh(source, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

template<typename T>
aru::standard::Array<T>& aru::standard::Array<T>::operator=(Array&& source) noexcept{
    if (this != &source){
        this->release();
        this->data = std::exchange(source.data, nullptr);
        this->count = std::exchange(source.count, std::size_t{0});
    }
    return *this;
}

template<typename T>
aru::standard::Array<T>& aru::standard::Array<T>::operator=(std::initializer_list<T> values){
    Array fresh(values);
    *this = std::move(fresh);
    return *this;
}

template<typename T>
aru::standard::Array<T> aru::standard::Array<T>::copy() const{
    return Array(*this, CopyTag{});
}

template<typename T>
aru::standard::Array<T> aru::standard::Array<T>::move() noexcept{
    return Array(std::move(*this), MoveTag{});
}

template<typename T>
T& aru::standard::Array<T>::operator[](std::size_t index) noexcept{
    return this->data[index];
}

template<typename T>
const T& aru::standard::Array<T>::operator[](std::size_t index) const noexcept{
    return this->data[index];
}

template<typename T>
std::size_t aru::standard::Array<T>::length() const noexcept{
    return this->count;
}

template<typename T>
bool aru::standard::Array<T>::empty() const noexcept{
    return this->count == 0;
}

template<typename T>
T* aru::standard::Array<T>::begin() noexcept{
    return this->data;
}

template<typename T>
const T* aru::standard::Array<T>::begin() const noexcept{
    return this->data;
}

template<typename T>
T* aru::standard::Array<T>::end() noexcept{
    return this->data != nullptr ? this->data + this->count : nullptr;
}

template<typename T>
const T* aru::standard::Array<T>::end() const noexcept{
    return this->data != nullptr ? this->data + this->count : nullptr;
}

template<typename T>
void aru::standard::Array<T>::release() noexcept{
    // Cek nullptr dulu sebelum destroy/deallocate.
    if (this->data != nullptr){
        std::destroy_n(this->data, this->count);
        std::allocator<T>{}.deallocate(this->data, this->count);
        this->data = nullptr;
    }
    this->count = 0;
}

template<typename T>
aru::standard::Array<T>::~Array<T>(){
    this->release();
}
