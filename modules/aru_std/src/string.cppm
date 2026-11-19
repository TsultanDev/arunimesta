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
    inline constexpr std::uint32_t replacementCodepoint = 0xFFFD;

    template<character T>
    std::size_t cstrLength(const T* str) noexcept {
        std::size_t length = 0;
        while (str[length] != T{}) {
            ++length;
        }
        return length;
    }

    // Dekode satu code point UTF-8, mengembalikan U+FFFD untuk sekuens tidak valid.
    std::uint32_t decodeUtf8(const std::vector<char8_t>& data, std::size_t& index) noexcept {
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
            return replacementCodepoint;
        }

        if (index + length > data.size()) {
            index = data.size();
            return replacementCodepoint;
        }

        for (std::size_t i = 1; i < length; ++i) {
            const std::uint8_t byte = static_cast<std::uint8_t>(data[index + i]);
            if ((byte & 0xC0) != 0x80) {
                ++index;
                return replacementCodepoint;
            }
            codepoint = (codepoint << 6) | (byte & 0x3F);
        }

        constexpr std::uint32_t minCodepoint[] = {0, 0x80, 0x800, 0x10000};
        const bool overlong = codepoint < minCodepoint[length - 1];
        const bool outOfRange = codepoint > 0x10FFFF;
        const bool surrogate = codepoint >= 0xD800 && codepoint <= 0xDFFF;
        if (overlong || outOfRange || surrogate) {
            index += length;
            return replacementCodepoint;
        }

        index += length;
        return codepoint;
    }

    // Dekode satu code point UTF-16, mengembalikan U+FFFD untuk sekuens tidak valid.
    std::uint32_t decodeUtf16(const std::vector<char16_t>& data, std::size_t& index) noexcept {
        const std::uint32_t unit = static_cast<std::uint32_t>(data[index]);

        if (unit < 0xD800 || unit > 0xDFFF) {
            ++index;
            return unit;
        }

        // Surrogate rendah tanpa surrogate tinggi sebelumnya.
        if (unit >= 0xDC00) {
            ++index;
            return replacementCodepoint;
        }

        // Surrogate tinggi, perlu pasangan surrogate rendah.
        if (index + 1 >= data.size()) {
            ++index;
            return replacementCodepoint;
        }

        const std::uint32_t low = static_cast<std::uint32_t>(data[index + 1]);
        if (low < 0xDC00 || low > 0xDFFF) {
            ++index;
            return replacementCodepoint;
        }

        index += 2;
        return 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
    }

    // Dekode satu code point UTF-32, mengembalikan U+FFFD untuk nilai tidak valid.
    std::uint32_t decodeUtf32(const std::vector<char32_t>& data, std::size_t& index) noexcept {
        const std::uint32_t codepoint = static_cast<std::uint32_t>(data[index]);
        ++index;

        const bool outOfRange = codepoint > 0x10FFFF;
        const bool surrogate = codepoint >= 0xD800 && codepoint <= 0xDFFF;
        if (outOfRange || surrogate) {
            return replacementCodepoint;
        }
        return codepoint;
    }

    // Enkode satu code point ke target, codepoint harus sudah valid.
    template<character T>
    void encodeCodepoint(std::vector<T>& out, std::uint32_t codepoint) {
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

        // Konversi antar encoding; SourceT == T berarti salinan (otomatis mengcopy).
        template<character SourceT>
        static String from(const String<SourceT>& str);

        // Rule of Five: copy/move constructor tersedia (tidak dihapus).
        String(const String& other);
        String(String&& other) noexcept;

        // Pindahkan isi string keluar; string asal menjadi kosong.
        // Tipe hasil selalu sama dengan string asal (aturan UTF8/16/32).
        String move() noexcept;

        const T* stringData() const noexcept;
        T* stringData() noexcept;

        std::uint32_t length() const noexcept;
        std::uint32_t byteSize() const noexcept;
        bool empty() const noexcept;

        const T& operator[](std::uint32_t index) const noexcept;
        T& operator[](std::uint32_t index) noexcept;

        auto begin() const noexcept -> typename std::vector<T>::const_iterator;
        auto end() const noexcept -> typename std::vector<T>::const_iterator;

        bool operator==(const String&) const noexcept;

        String& operator=(const String&);
        String& operator=(String&&) noexcept;
        ~String();

    private:
        // Semua instansiasi String saling berteman, agar from<SourceT>()
        // dapat mengakses data private dari String<SourceT>.
        template<character U>
        friend class String;

        String();
        String(const T* str);
        explicit String(std::vector<T> strData);

        std::uint32_t charSize = {};  // jumlah code unit (tanpa terminator)
        std::uint32_t size = {};      // jumlah byte (charSize * sizeof(T))
        std::vector<T> data;          // selalu diakhiri terminator null
};

template<aru::standard::character T>
aru::standard::String<T>::String() : charSize(0), size(0), data(1, T{}) {}

template<aru::standard::character T>
aru::standard::String<T>::String(const T* str) : charSize(0), size(0), data() {
    this->charSize = static_cast<std::uint32_t>(detail::cstrLength(str));
    this->size = this->charSize * sizeof(T);

    this->data.reserve(static_cast<std::size_t>(this->charSize) + 1);
    for (std::uint32_t i = 0; i < this->charSize; ++i) {
        this->data.push_back(str[i]);
    }
    this->data.push_back(T{});
}

template<aru::standard::character T>
aru::standard::String<T>::String(std::vector<T> strData) :
    charSize(static_cast<std::uint32_t>(strData.size()) - 1),
    size(this->charSize * sizeof(T)),
    data(std::move(strData))
{}

template<aru::standard::character T>
aru::standard::String<T>::String(const String& other) = default;

template<aru::standard::character T>
aru::standard::String<T>::String(String&& other) noexcept :
    charSize(std::exchange(other.charSize, std::uint32_t{0})),
    size(std::exchange(other.size, std::uint32_t{0})),
    data(std::exchange(other.data, std::vector<T>{}))
{}

template<aru::standard::character T>
aru::standard::String<T>& aru::standard::String<T>::operator=(const String&) = default;

template<aru::standard::character T>
aru::standard::String<T>& aru::standard::String<T>::operator=(String&& other) noexcept{
    if (this != &other){
        this->charSize = std::exchange(other.charSize, std::uint32_t{0});
        this->size = std::exchange(other.size, std::uint32_t{0});
        this->data = std::exchange(other.data, std::vector<T>{});
    }
    return *this;
}

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
        out.reserve(str.charSize);
        std::size_t index = 0;
        while (index < str.charSize) {
            std::uint32_t codepoint = 0;
            if constexpr (std::same_as<SourceT, char8_t>) {
                codepoint = detail::decodeUtf8(str.data, index);
            } else if constexpr (std::same_as<SourceT, char16_t>) {
                codepoint = detail::decodeUtf16(str.data, index);
            } else {
                codepoint = detail::decodeUtf32(str.data, index);
            }
            detail::encodeCodepoint(out, codepoint);
        }
        out.push_back(T{});
    }
    return String<T>(std::move(out));
}

template<aru::standard::character T>
aru::standard::String<T> aru::standard::String<T>::move() noexcept{
    std::vector<T> stolen = std::exchange(this->data, std::vector<T>());
    this->charSize = 0;
    this->size = 0;
    return String<T>(std::move(stolen));
}

template<aru::standard::character T>
const T* aru::standard::String<T>::stringData() const noexcept{
    return this->data.data();
}

template<aru::standard::character T>
T* aru::standard::String<T>::stringData() noexcept{
    return this->data.data();
}

template<aru::standard::character T>
std::uint32_t aru::standard::String<T>::length() const noexcept{
    return this->charSize;
}

template<aru::standard::character T>
std::uint32_t aru::standard::String<T>::byteSize() const noexcept{
    return this->size;
}

template<aru::standard::character T>
bool aru::standard::String<T>::empty() const noexcept{
    return this->charSize == 0;
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
