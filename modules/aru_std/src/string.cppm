module;
#include <cstdint>
#include <vector>
#include <winsock.h>
export module aru.standard:string;

export namespace aru{
    namespace standard{
        using str = const char*;
        template<typename T> requires (
            std::same_as<T, char> || std::same_as<T, const char8_t> ||
            std::same_as<T, wchar_t> || std::same_as<T, const char16_t> ||
            std::same_as<T, char32_t> || std::same_as<T, const u_char>
        )
        class String;
    }
}

template<typename T>
requires (
    std::same_as<T, char> || std::same_as<T, const char8_t> ||
    std::same_as<T, wchar_t> || std::same_as<T, const char16_t> ||
    std::same_as<T, char32_t> || std::same_as<T, const u_char>
)
class aru::standard::String{
    public:
        static String from(const T* str);

        template <typename TargetT> requires (
            std::same_as<T, char> || std::same_as<T, const char8_t> ||
            std::same_as<T, wchar_t> || std::same_as<T, const char16_t> ||
            std::same_as<T, char32_t> || std::same_as<T, const u_char>
        )
        String<TargetT> encode() const;

        const T& string_data() const;
        T& string_data() noexcept;

        String(const T*);
        ~String();

    private:
        uint32_t char_size = {};
        uint32_t size = {};
        std::vector<T> data;
};

template<typename T>
requires (
    std::same_as<T, char> || std::same_as<T, const char8_t> ||
    std::same_as<T, wchar_t> || std::same_as<T, const char16_t> ||
    std::same_as<T, char32_t> || std::same_as<T, const u_char>
)
aru::standard::String aru::standard::String::from(const T* str){

}

template<typename T>
aru::standard::String aru::standard::String::encode<T>() const{

}

template<typename T>
const T& aru::standard::String::data() const;

template<typename T>
T& aru::standard::String::data() noexcept;

template<typename T>
aru::standard::String::String(const T*){

}

template<typename T>
aru::standard::String::~String<T>(){

}
