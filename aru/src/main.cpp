#include <print>
import aru;

#include <cstdio>
#include <cstdlib>

int main() {
    auto sEntry = renaru::Result::isSuccess(
        renaru::Entry::init(),
        [](renaru::Error sError) noexcept -> renaru::Entry {
            std::println(stderr, "Entry init failed: %u\n", static_cast<unsigned int>(sError));
            std::exit(1);
        });

    return 0;
}
