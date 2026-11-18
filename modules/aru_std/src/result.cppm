module;
#include <concepts>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
export module aru.standard:result;

export namespace aru{
    namespace standard{
        template <typename T, typename E>
        class Result;

        // Tag untuk membangun error di dalam union tanpa perlu copy/move E.
        struct errorInPlace_t {
            explicit errorInPlace_t() = default;
        };
        inline constexpr errorInPlace_t errorInPlace{};
    }
}

template <typename T, typename E>
class aru::standard::Result{
    private:
        bool has_value = {};
        union{
            T value;
            E error;
        };
    public:

        Result(const T& value);
        Result(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>);
        Result(const E& error);
        Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>);

        // Bangun value/error langsung di dalam union. Berguna untuk tipe
        // yang sengaja menghapus copy/move constructor (Window, String, ...).
        template <typename... Args>
        Result(std::in_place_t, Args&&... args);

        template <typename... Args>
        Result(aru::standard::errorInPlace_t, Args&&... args);

        ~Result();

        bool isOkOrErr() const noexcept;
        explicit operator bool() const noexcept;

        template <typename Func>
            requires std::invocable<Func&&, E&&>
        T valueOrUnexpected(Func&& fn);
};

template<typename T, typename E>
aru::standard::Result<T, E>::Result(const T& value) : has_value(true), value(value){}

template<typename T, typename E>
aru::standard::Result<T, E>::Result(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) : has_value(true), value(std::move(value)){}

template<typename T, typename E>
aru::standard::Result<T, E>::Result(const E& error) : has_value(false), error(error){}

template<typename T, typename E>
aru::standard::Result<T, E>::Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>) : has_value(false), error(std::move(error)){}

template<typename T, typename E>
template<typename... Args>
aru::standard::Result<T, E>::Result(std::in_place_t, Args&&... args) :
    has_value(true),
    value(std::forward<Args>(args)...)
{}

template<typename T, typename E>
template<typename... Args>
aru::standard::Result<T, E>::Result(aru::standard::errorInPlace_t, Args&&... args) :
    has_value(false),
    error(std::forward<Args>(args)...)
{}

template<typename T, typename E>
aru::standard::Result<T, E>::~Result<T, E>() {
    if (this->has_value) {
        std::destroy_at(&this->value);
    } else {
        std::destroy_at(&this->error);
    }
}

template<typename T, typename E>
bool aru::standard::Result<T, E>::isOkOrErr() const noexcept{
    return this->has_value;
}

template<typename T, typename E>
aru::standard::Result<T, E>::operator bool() const noexcept{
    return this->has_value;
}

template<typename T, typename E>
template <typename Func>
    requires std::invocable<Func&&, E&&>
T aru::standard::Result<T, E>::valueOrUnexpected(Func&& fn){
    if (this->has_value) {
        // Tipe dengan move constructor dihapus (String, Vector, Window)
        // menyediakan move() prvalue; guaranteed copy elision menangani sisanya.
        if constexpr (requires(T& stored) { stored.move(); }) {
            return this->value.move();
        } else {
            return std::move(this->value);
        }
    }

    using CallbackResult = std::invoke_result_t<Func&&, E&&>;
    if constexpr (std::is_void_v<CallbackResult>) {
        // Callback bertindak sebagai error handler; tidak ada T untuk
        // dikembalikan, jadi hentikan eksekusi setelah handler dipanggil.
        std::invoke(std::forward<Func>(fn), std::move(this->error));
        std::terminate();
    } else {
        return std::invoke(std::forward<Func>(fn), std::move(this->error));
    }
}
