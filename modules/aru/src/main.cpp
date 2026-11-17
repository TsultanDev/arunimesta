#include <cstdint>
#include <expected>
#include <print>
import aru;

int main() {
    auto string = aru::standard::String<char8_t>::from(u8"Hello");
    auto s2 = aru::standard::String<char32_t>::from(string);
    auto vec = aru::standard::Vector<uint64_t>::create(16);
    vec = {1, 3, 4};
    return 0;
}
