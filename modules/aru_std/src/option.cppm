module;

#include <concepts>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

export module aru.standard:option;

export namespace aru::standard {
    template<typename T>
    class Option;

    // Dipakai andThen() untuk memastikan callback mengembalikan Option<U>.
    template<typename T>
    struct isOption : std::false_type {};

    template<typename T>
    struct isOption<Option<T>> : std::true_type {};

    template<typename T>
    inline constexpr bool isOptionV = isOption<std::remove_cvref_t<T>>::value;
}

template<typename T>
class aru::standard::Option {
    private:
        bool active = false;
        union {
            T mValue;
        };

        struct NoneTag {};
        struct CopyTag {};
        struct MoveTag {};

        Option(NoneTag) noexcept;
        Option(const T& value);
        Option(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>);
        Option(const Option& other, CopyTag);
        Option(Option&& other, MoveTag) noexcept(std::is_nothrow_move_constructible_v<T>);

    public:
        // Pembuatan lewat factory, seperti String::from() / Vector::create().
        static Option some(const T& value);
        static Option some(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>);

        template<typename... Args>
        static Option someInPlace(Args&&... args);

        static Option none() noexcept;

        // Rule of Five: copy menyalin dalam, move mencuri nilai dan mengosongkan source.
        Option(const Option& other);
        Option(Option&& other) noexcept(std::is_nothrow_move_constructible_v<T>);
        Option& operator=(const Option& other);
        Option& operator=(Option&& other) noexcept(std::is_nothrow_move_constructible_v<T>);
        ~Option();

        Option copy() const;
        Option move() noexcept(std::is_nothrow_move_constructible_v<T>);

        bool isSome() const noexcept;
        bool isNone() const noexcept;
        explicit operator bool() const noexcept;

        T& value() &;
        const T& value() const&;
        T&& value() &&;

        T unwrap();
        T unwrapOr(T fallback);

        // Callback dipanggil saat None; mengikuti pola Result::valueOrUnexpected().
        template<typename Func>
            requires std::invocable<Func&&>
        T valueOr(Func&& fn);

        template<typename Func>
            requires std::invocable<Func&&, T>
        auto map(Func&& fn) -> Option<std::invoke_result_t<Func&&, T>>;

        template<typename Func>
            requires std::invocable<Func&&, T> &&
                     aru::standard::isOptionV<std::invoke_result_t<Func&&, T>>
        auto andThen(Func&& fn) -> std::invoke_result_t<Func&&, T>;

        template<typename Func>
            requires std::invocable<Func&&>
        auto orElse(Func&& fn) -> Option<T>;

        void reset() noexcept;
        Option take() noexcept(std::is_nothrow_move_constructible_v<T>);
};

template<typename T>
aru::standard::Option<T> aru::standard::Option<T>::some(const T& value) {
    return Option(value);
}

template<typename T>
aru::standard::Option<T> aru::standard::Option<T>::some(T&& value)
    noexcept(std::is_nothrow_move_constructible_v<T>)
{
    return Option(std::move(value));
}

template<typename T>
template<typename... Args>
aru::standard::Option<T> aru::standard::Option<T>::someInPlace(Args&&... args) {
    Option result(NoneTag{});
    std::construct_at(&result.mValue, std::forward<Args>(args)...);
    result.active = true;
    return result;
}

template<typename T>
aru::standard::Option<T> aru::standard::Option<T>::none() noexcept {
    return Option(NoneTag{});
}

template<typename T>
aru::standard::Option<T>::Option(NoneTag) noexcept : active(false) {}

template<typename T>
aru::standard::Option<T>::Option(const T& value) : active(true), mValue(value) {}

template<typename T>
aru::standard::Option<T>::Option(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) :
    active(true),
    mValue(std::move(value))
{}

template<typename T>
aru::standard::Option<T>::Option(const Option& other) :
    Option(other, CopyTag{})
{}

template<typename T>
aru::standard::Option<T>::Option(Option&& other) noexcept(std::is_nothrow_move_constructible_v<T>) :
    Option(std::move(other), MoveTag{})
{}

template<typename T>
aru::standard::Option<T>::Option(const Option& other, CopyTag) : active(false) {
    if (other.active) {
        std::construct_at(&this->mValue, other.mValue);
        this->active = true;
    }
}

template<typename T>
aru::standard::Option<T>::Option(Option&& other, MoveTag) noexcept(std::is_nothrow_move_constructible_v<T>) :
    active(false)
{
    if (other.active) {
        std::construct_at(&this->mValue, std::move(other.mValue));
        this->active = true;
        other.reset();
    }
}

template<typename T>
aru::standard::Option<T>& aru::standard::Option<T>::operator=(const Option& other) {
    if (this == &other) {
        return *this;
    }
    Option fresh(other, CopyTag{});
    *this = std::move(fresh);
    return *this;
}

template<typename T>
aru::standard::Option<T>& aru::standard::Option<T>::operator=(Option&& other)
    noexcept(std::is_nothrow_move_constructible_v<T>)
{
    if (this == &other) {
        return *this;
    }
    this->reset();
    if (other.active) {
        std::construct_at(&this->mValue, std::move(other.mValue));
        this->active = true;
        other.reset();
    }
    return *this;
}

template<typename T>
aru::standard::Option<T>::~Option<T>() {
    this->reset();
}

template<typename T>
aru::standard::Option<T> aru::standard::Option<T>::copy() const {
    return Option(*this, CopyTag{});
}

template<typename T>
aru::standard::Option<T> aru::standard::Option<T>::move()
    noexcept(std::is_nothrow_move_constructible_v<T>)
{
    return Option(std::move(*this), MoveTag{});
}

template<typename T>
bool aru::standard::Option<T>::isSome() const noexcept {
    return this->active;
}

template<typename T>
bool aru::standard::Option<T>::isNone() const noexcept {
    return !this->active;
}

template<typename T>
aru::standard::Option<T>::operator bool() const noexcept {
    return this->active;
}

template<typename T>
T& aru::standard::Option<T>::value() & {
    return this->mValue;
}

template<typename T>
const T& aru::standard::Option<T>::value() const& {
    return this->mValue;
}

template<typename T>
T&& aru::standard::Option<T>::value() && {
    return std::move(this->mValue);
}

template<typename T>
T aru::standard::Option<T>::unwrap() {
    if (!this->active) {
        std::terminate();
    }
    if constexpr (requires(T& stored) { stored.move(); }) {
        T result = this->mValue.move();
        this->reset();
        return result;
    } else {
        T result = std::move(this->mValue);
        this->reset();
        return result;
    }
}

template<typename T>
T aru::standard::Option<T>::unwrapOr(T fallback) {
    if (this->active) {
        return this->unwrap();
    }
    return fallback;
}

template<typename T>
template<typename Func>
    requires std::invocable<Func&&>
T aru::standard::Option<T>::valueOr(Func&& fn) {
    if (this->active) {
        return this->unwrap();
    }
    using CallbackResult = std::invoke_result_t<Func&&>;
    if constexpr (std::is_void_v<CallbackResult>) {
        // Callback bertindak sebagai penangan None; eksekusi dihentikan setelahnya.
        std::invoke(std::forward<Func>(fn));
        std::terminate();
    } else {
        return std::invoke(std::forward<Func>(fn));
    }
}

template<typename T>
template<typename Func>
    requires std::invocable<Func&&, T>
auto aru::standard::Option<T>::map(Func&& fn) -> Option<std::invoke_result_t<Func&&, T>> {
    using U = std::invoke_result_t<Func&&, T>;
    if (!this->active) {
        return Option<U>::none();
    }
    auto mapped = Option<U>::someInPlace(std::invoke(std::forward<Func>(fn), std::move(this->mValue)));
    this->reset();
    return mapped;
}

template<typename T>
template<typename Func>
    requires std::invocable<Func&&, T> &&
             aru::standard::isOptionV<std::invoke_result_t<Func&&, T>>
auto aru::standard::Option<T>::andThen(Func&& fn) -> std::invoke_result_t<Func&&, T> {
    using ResultOption = std::invoke_result_t<Func&&, T>;
    if (!this->active) {
        return ResultOption::none();
    }
    auto result = std::invoke(std::forward<Func>(fn), std::move(this->mValue));
    this->reset();
    return result;
}

template<typename T>
template<typename Func>
    requires std::invocable<Func&&>
auto aru::standard::Option<T>::orElse(Func&& fn) -> Option<T> {
    if (this->active) {
        return this->move();
    }
    return std::invoke(std::forward<Func>(fn));
}

template<typename T>
void aru::standard::Option<T>::reset() noexcept {
    if (this->active) {
        std::destroy_at(&this->mValue);
        this->active = false;
    }
}

template<typename T>
aru::standard::Option<T> aru::standard::Option<T>::take()
    noexcept(std::is_nothrow_move_constructible_v<T>)
{
    return this->move();
}
