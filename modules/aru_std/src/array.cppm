module;

#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <utility>

export module aru.standard:array;

export namespace aru::standard{
    template<typename T, std::size_t N>
    class Array;
}

// Array ukuran tetap dengan storage inline T data[N]. Tidak ada alokasi heap,
// allocator, atau manual construct/destroy. Ukuran N adalah bagian dari tipe.
template<typename T, std::size_t N>
class aru::standard::Array{
    public:
        using value_type = T;
        static constexpr std::size_t extent = N;

        static_assert(N > 0, "aru::standard::Array requires N > 0");

        // Semua elemen value-initialized (T{}).
        static Array create();

        // Isi dari literal; sisanya diisi T{}. Lempar std::length_error
        // bila jumlah initializer melebihi N.
        static Array create(std::initializer_list<T> values);

        // Literal constructor: Array<T, N> a = {1, 2, 3};
        Array(std::initializer_list<T> values);

        // Rule of Five defaulted: dengan storage inline, copy menyalin elemen
        // dan move memindahkan elemen satu per satu (sumber tetap valid).
        Array(const Array&) = default;
        Array(Array&&) = default;
        Array& operator=(const Array&) = default;
        Array& operator=(Array&&) = default;
        ~Array() = default;

        Array& operator=(std::initializer_list<T> values);

        Array copy() const;
        Array move();

        T& operator[](std::size_t index) noexcept;
        const T& operator[](std::size_t index) const noexcept;

        std::size_t length() const noexcept;
        bool empty() const noexcept;

        T* begin() noexcept;
        const T* begin() const noexcept;
        T* end() noexcept;
        const T* end() const noexcept;

    private:
        // Default constructor tetap privat: pembuatan lewat create().
        Array() = default;

        T data[N]{};
};

template<typename T, std::size_t N>
aru::standard::Array<T, N> aru::standard::Array<T, N>::create(){
    return Array{};
}

template<typename T, std::size_t N>
aru::standard::Array<T, N> aru::standard::Array<T, N>::create(std::initializer_list<T> values){
    return Array(values);
}

template<typename T, std::size_t N>
aru::standard::Array<T, N>::Array(std::initializer_list<T> values){
    if (values.size() > N){
        throw std::length_error("aru::standard::Array: too many initializers");
    }

    std::size_t index = 0;
    for (const T& value : values){
        this->data[index++] = value;
    }
    for (; index < N; ++index){
        this->data[index] = T{};
    }
}

template<typename T, std::size_t N>
aru::standard::Array<T, N>& aru::standard::Array<T, N>::operator=(std::initializer_list<T> values){
    Array fresh(values);
    *this = std::move(fresh);
    return *this;
}

template<typename T, std::size_t N>
aru::standard::Array<T, N> aru::standard::Array<T, N>::copy() const{
    return Array(*this);
}

template<typename T, std::size_t N>
aru::standard::Array<T, N> aru::standard::Array<T, N>::move(){
    return Array(std::move(*this));
}

template<typename T, std::size_t N>
T& aru::standard::Array<T, N>::operator[](std::size_t index) noexcept{
    return this->data[index];
}

template<typename T, std::size_t N>
const T& aru::standard::Array<T, N>::operator[](std::size_t index) const noexcept{
    return this->data[index];
}

template<typename T, std::size_t N>
std::size_t aru::standard::Array<T, N>::length() const noexcept{
    return N;
}

template<typename T, std::size_t N>
bool aru::standard::Array<T, N>::empty() const noexcept{
    return false;
}

template<typename T, std::size_t N>
T* aru::standard::Array<T, N>::begin() noexcept{
    return this->data;
}

template<typename T, std::size_t N>
const T* aru::standard::Array<T, N>::begin() const noexcept{
    return this->data;
}

template<typename T, std::size_t N>
T* aru::standard::Array<T, N>::end() noexcept{
    return this->data + N;
}

template<typename T, std::size_t N>
const T* aru::standard::Array<T, N>::end() const noexcept{
    return this->data + N;
}
