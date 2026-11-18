#include <array>
#include <cstdint>
#include <expected>
#include <print>
#include <vector>
import aru;

int main() {
    auto vec = aru::standard::Vector<uint64_t>::create(5);
    aru::standard::Vector<int> vec2 = {1, 2, 3};
    auto vec3 = {1,2,3};

    aru::standard::Vector<int> vec4 = vec3;

    aru::standard::Vector<int> vec5 = vec4.copy();
    auto vec6 = aru::standard::Vector<int>::create({1, 2, 3});

    std::array<int, 4> arr = {1,2,3,4};
    auto vec6 = aru::standard::Vector<int>::create(arr);


    return 0;
}
