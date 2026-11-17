module;
#include <concepts>
#include <memory>
#include <type_traits>
export module aru.standard:result;

export namespace aru{
    namespace standard{
        template <typename T, typename E>
        class Result{
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
                Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<T>);

                ~Result();

                bool is_ok_or_err() const noexcept;
                explicit operator bool() const noexcept;

                template <std::invocable<const E&> Func>
                T value_or_unexpected(Func&& fn);


        };
    }
}

template<typename T, typename E>
aru::standard::Result<T, E>::Result(const T& value) : has_value(true), value(value){}

template<typename T, typename E>
aru::standard::Result<T, E>::Result(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) : has_value(true), value(std::move(value)){}

template<typename T, typename E>
aru::standard::Result<T, E>::Result(const E& error) : has_value(false), error(error){}

template<typename T, typename E>
aru::standard::Result<T, E>::Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<T>) : has_value(false), error(error){}

template<typename T, typename E>
aru::standard::Result<T, E>::~Result<T, E>() {
    if (this->has_value) {
        std::destroy_at(&this->value);
    } else {
        std::destroy_at(&this->error);
    }
}

template<typename T, typename E>
bool aru::standard::Result<T, E>::is_ok_or_err() const noexcept{
    return this->has_value;
}

template<typename T, typename E>
template <std::invocable<const E&> Func>
T aru::standard::Result<T, E>::value_or_unexpected(Func&& fn){
    if (has_value) {
        return std::move(value);
    }
    return std::invoke(std::forward<Func>(fn), error);
}
