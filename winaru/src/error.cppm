module;

#include <cstdint>

export module winaru:error;

export namespace winaru {
    enum class Error : std::uint32_t {
        eNone                           = 0x00000000,
        eClassRegistrationFailed        = 0x00000001,
        eWindowCreationFailed           = 0x00000002,
        eMaxEnum                        = 0xFFFFFFFF
    };
}
