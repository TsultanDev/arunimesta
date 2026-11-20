module;

#include <format>
#include <string_view>

export module aru.window:error;

export namespace aru::window {
    enum class Error {
        None = 0,
        ClassRegistrationFailed = 1,
        WindowCreationFailed = 2,
    };
}

// Supaya println("Error! : {}", err) bisa mencetak Error langsung.
export namespace std {
    template<>
    struct formatter<aru::window::Error> : formatter<string_view> {
        template<typename FormatContext>
        auto format(const aru::window::Error& error, FormatContext& ctx) const {
            string_view text = "unknown";
            switch (error) {
                case aru::window::Error::None:
                    text = "none";
                    break;
                case aru::window::Error::ClassRegistrationFailed:
                    text = "class registration failed";
                    break;
                case aru::window::Error::WindowCreationFailed:
                    text = "window creation failed";
                    break;
            }
            return formatter<string_view>::format(text, ctx);
        }
    };
}
