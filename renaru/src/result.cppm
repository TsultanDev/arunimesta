module;

#include <expected>
#include <functional>
#include <type_traits>
#include <utility>

export module renaru:result;

export namespace renaru {
    class Result {
        public:
            // Meng-unwrap std::expected: sukses → objek, gagal → jalankan Fn.
            template <typename T, typename E, typename Fn>
            static T isSuccess(std::expected<T, E> sExpected, Fn&& sOnError)
                noexcept(std::is_nothrow_invocable_v<Fn, E> &&
                         std::is_nothrow_move_constructible_v<T>){
                if (sExpected.has_value()){
                    return std::move(*sExpected);
                }
                return std::invoke(std::forward<Fn>(sOnError), sExpected.error());
            }
    };
}
