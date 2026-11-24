module;

#include <format>
#include <string_view>

export module winaru:error;

export namespace winaru {
    enum class Error {
        None = 0,
        ClassRegistrationFailed = 1,
        WindowCreationFailed = 2,
    };
}

// Supaya println("Error! : {}", err) bisa mencetak Error langsung.
export namespace std {
    template<>
    struct formatter<winaru::Error> : formatter<string_view> {
        template<typename FormatContext>
        auto format(const winaru::Error& sError, FormatContext& ctx) const {
            string_view text = "unknown";
            switch (sError) {
                case winaru::Error::None:
                    text = "none";
                    break;
                case winaru::Error::ClassRegistrationFailed:
                    text = "class registration failed";
                    break;
                case winaru::Error::WindowCreationFailed:
                    text = "window creation failed";
                    break;
            }
            return formatter<string_view>::format(text, ctx);
        }
    };
}
