module;

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

export module aru.standard:string;

export namespace aru{
    namespace standard{
        template<typename T>
        concept character = (
            std::same_as<T, char8_t> || std::same_as<T, char16_t> ||
            std::same_as<T, char32_t>
        );

        template<character T>
        class String;
    }
}

namespace aru::standard::detail{
    inline constexpr std::uint32_t replacement_codepoint = 0xFFFD;

    template<character T>
    std::size_t cstr_length(const T* str) noexcept {
        std::size_t length = 0;
        while (str[length] != T{}) {
            ++length;
        }
        return length;
    }

    std::uint32_t decode_utf8(const std::vector<char8_t>& data, std::size_t& index) noexcept {
        const std::uint8_t first = static_cast<std::uint8_t>(data[index]);
        std::uint32_t codepoint = 0;
        std::size_t length = 0;

        if (first < 0x80) {
            codepoint = first;
            length = 1;
        } else if ((first & 0xE0) == 0xC0) {
            codepoint = first & 0x1F;
            length = 2;
        } else if ((first & 0xF0) == 0xE0) {
            codepoint = first & 0x0F;
            length = 3;
        } else if ((first & 0xF8) == 0xF0) {
            codepoint = first & 0x07;
            length = 4;
        } else {
            ++index;
            return replacement_codepoint;
        }

        if (index + length > data.size()) {
            index = data.size();
            return replacement_codepoint;
        }

        for (std::size_t i = 1; i < length; ++i) {
            const std::uint8_t byte = static_cast<std::uint8_t>(data[index + i]);
            if ((byte & 0xC0) != 0x80) {
                ++index;
                return replacement_codepoint;
            }
            codepoint = (codepoint << 6) | (byte & 0x3F);
        }

        constexpr std::uint32_t min_codepoint[] = {0, 0x80, 0x800, 0x10000};
        const bool overlong = codepoint < min_codepoint[length - 1];
        const bool out_of_range = codepoint > 0x10FFFF;
        const bool surrogate = codepoint >= 0xD800 && codepoint <= 0xDFFF;
        if (overlong || out_of_range || surrogate) {
            index += length;
            return replacement_codepoint;
        }

        index += length;
        return codepoint;
    }

    std::uint32_t decode_utf16(const std::vector<char16_t>& data, std::size_t& index) noexcept {
        const std::uint32_t unit = static_cast<std::uint32_t>(data[index]);

        if (unit < 0xD800 || unit > 0xDFFF) {
            ++index;
            return unit;
        }

        if (unit >= 0xDC00) {
            ++index;
            return replacement_codepoint;
        }

        if (index + 1 >= data.size()) {
            ++index;
            return replacement_codepoint;
        }

        const std::uint32_t low = static_cast<std::uint32_t>(data[index + 1]);
        if (low < 0xDC00 || low > 0xDFFF) {
            ++index;
            return replacement_codepoint;
        }

        index += 2;
        return 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
    }

    std::uint32_t decode_utf32(const std::vector<char32_t>& data, std::size_t& index) noexcept {
        const std::uint32_t codepoint = static_cast<std::uint32_t>(data[index]);
        ++index;

        const bool out_of_range = codepoint > 0x10FFFF;
        const bool surrogate = codepoint >= 0xD800 && codepoint <= 0xDFFF;
        if (out_of_range || surrogate) {
            return replacement_codepoint;
        }
        return codepoint;
    }

    template<character T>
    void encode_codepoint(std::vector<T>& out, std::uint32_t codepoint) {
        if constexpr (std::same_as<T, char8_t>) {
            if (codepoint < 0x80) {
                out.push_back(static_cast<char8_t>(codepoint));
            } else if (codepoint < 0x800) {
                out.push_back(static_cast<char8_t>(0xC0 | (codepoint >> 6)));
                out.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
            } else if (codepoint < 0x10000) {
                out.push_back(static_cast<char8_t>(0xE0 | (codepoint >> 12)));
                out.push_back(static_cast<char8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                out.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
            } else {
                out.push_back(static_cast<char8_t>(0xF0 | (codepoint >> 18)));
                out.push_back(static_cast<char8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
                out.push_back(static_cast<char8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                out.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
            }
        } else if constexpr (std::same_as<T, char16_t>) {
            if (codepoint < 0x10000) {
                out.push_back(static_cast<char16_t>(codepoint));
            } else {
                codepoint -= 0x10000;
                out.push_back(static_cast<char16_t>(0xD800 | (codepoint >> 10)));
                out.push_back(static_cast<char16_t>(0xDC00 | (codepoint & 0x3FF)));
            }
        } else {
            out.push_back(static_cast<char32_t>(codepoint));
        }
    }
}

template<aru::standard::character T>
class aru::standard::String{
    public:
        using value_type = T;

        static String from(const T* str);

        template<character SourceT>
        static String from(const String<SourceT>& str);

        const T* string_data() const noexcept;
        T* string_data() noexcept;

        std::uint32_t length() const noexcept;
        std::uint32_t byte_size() const noexcept;
        bool empty() const noexcept;

        const T& operator[](std::uint32_t index) const noexcept;
        T& operator[](std::uint32_t index) noexcept;

        auto begin() const noexcept -> typename std::vector<T>::const_iterator;
        auto end() const noexcept -> typename std::vector<T>::const_iterator;

        bool operator==(const String&) const noexcept;

        String();
        String(const T*);
        String(const String&);
        String(String&&) noexcept;
        String& operator=(const String&);
        String& operator=(String&&) noexcept;
        ~String();

    private:
        template<character U>
        friend class String;

        explicit String(std::vector<T> str_data);

        std::uint32_t char_size = {};
        std::uint32_t size = {};
        std::vector<T> data;
};

template<aru::standard::character T>
aru::standard::String<T>::String() : char_size(0), size(0), data(1, T{}) {}

template<aru::standard::character T>
aru::standard::String<T>::String(const T* str) : char_size(0), size(0), data() {
    this->char_size = static_cast<std::uint32_t>(detail::cstr_length(str));
    this->size = this->char_size * sizeof(T);

    this->data.reserve(static_cast<std::size_t>(this->char_size) + 1);
    for (std::uint32_t i = 0; i < this->char_size; ++i) {
        this->data.push_back(str[i]);
    }
    this->data.push_back(T{});
}

template<aru::standard::character T>
aru::standard::String<T>::String(std::vector<T> str_data) :
    char_size(static_cast<std::uint32_t>(str_data.size()) - 1),
    size(this->char_size * sizeof(T)),
    data(std::move(str_data))
{}

template<aru::standard::character T>
aru::standard::String<T>::String(const String&) = default;

template<aru::standard::character T>
aru::standard::String<T>::String(String&&) noexcept = default;

template<aru::standard::character T>
aru::standard::String<T>& aru::standard::String<T>::operator=(const String&) = default;

template<aru::standard::character T>
aru::standard::String<T>& aru::standard::String<T>::operator=(String&&) noexcept = default;

template<aru::standard::character T>
aru::standard::String<T>::~String<T>() = default;

template<aru::standard::character T>
aru::standard::String<T> aru::standard::String<T>::from(const T* str){
    return String<T>(str);
}

template<aru::standard::character T>
template<aru::standard::character SourceT>
aru::standard::String<T> aru::standard::String<T>::from(const String<SourceT>& str){
    std::vector<T> out;

    if constexpr (std::same_as<T, SourceT>) {
        out = str.data;  // sudah termasuk terminator
    } else {
        out.reserve(str.char_size);
        std::size_t index = 0;
        while (index < str.char_size) {
            std::uint32_t codepoint = 0;
            if constexpr (std::same_as<SourceT, char8_t>) {
                codepoint = detail::decode_utf8(str.data, index);
            } else if constexpr (std::same_as<SourceT, char16_t>) {
                codepoint = detail::decode_utf16(str.data, index);
            } else {
                codepoint = detail::decode_utf32(str.data, index);
            }
            detail::encode_codepoint(out, codepoint);
        }
        out.push_back(T{});
    }
    return String<T>(std::move(out));
}

template<aru::standard::character T>
const T* aru::standard::String<T>::string_data() const noexcept{
    return this->data.data();
}

template<aru::standard::character T>
T* aru::standard::String<T>::string_data() noexcept{
    return this->data.data();
}

template<aru::standard::character T>
std::uint32_t aru::standard::String<T>::length() const noexcept{
    return this->char_size;
}

template<aru::standard::character T>
std::uint32_t aru::standard::String<T>::byte_size() const noexcept{
    return this->size;
}

template<aru::standard::character T>
bool aru::standard::String<T>::empty() const noexcept{
    return this->char_size == 0;
}

template<aru::standard::character T>
const T& aru::standard::String<T>::operator[](std::uint32_t index) const noexcept{
    return this->data[index];
}

template<aru::standard::character T>
T& aru::standard::String<T>::operator[](std::uint32_t index) noexcept{
    return this->data[index];
}

template<aru::standard::character T>
auto aru::standard::String<T>::begin() const noexcept -> typename std::vector<T>::const_iterator{
    return this->data.cbegin();
}

template<aru::standard::character T>
auto aru::standard::String<T>::end() const noexcept -> typename std::vector<T>::const_iterator{
    return this->data.cend() - 1;
}

template<aru::standard::character T>
bool aru::standard::String<T>::operator==(const String& other) const noexcept{
    return this->data == other.data;
}
