module;
#include <Windows.h>
#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

export module aru.window:window;
import aru.standard;

export namespace aru {
    namespace window {
        enum class Error;
        class Window;
    }
}

enum class aru::window::Error {
    None = 0,
    ClassRegistrationFailed = 1,
    WindowCreationFailed = 2,
};

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

class aru::window::Window{
    public:
        using ResultType = aru::standard::Result<Window, Error>;

        // Factory pembuat OS window baru.
        static ResultType create(const char* title, std::uint32_t width, std::uint32_t height);
        static ResultType create(const char16_t* title, std::uint32_t width, std::uint32_t height);

        // Membuat OS window BARU memakai konfigurasi sumber.
        // Ini bukan copy objek: HWND baru dibuat melalui WinAPI.
        static ResultType createFromWindow(const Window& source);

        // Rule of Five: copy menyalin metadata (bukan HWND), move memindahkan handle.
        Window(const Window& other);
        Window(Window&& other) noexcept;
        Window& operator=(const Window& source);
        Window& operator=(Window&& source) noexcept;

        // Pengganti copy/move constructor (prvalue, copy elision).
        Window copy() const;
        Window move() noexcept;

        void event_loop();
        ~Window();

        HWND handle() const noexcept;
        const aru::standard::String<char16_t>& windowTitle() const noexcept;
        std::uint32_t windowWidth() const noexcept;
        std::uint32_t windowHeight() const noexcept;
        bool empty() const noexcept;

        // Penanda untuk loop utama: true sampai WM_QUIT diterima event_loop().
        bool isOpen() const noexcept;
        bool shouldClose() const noexcept;

    private:
        struct AdoptTag {};
        struct CopyTag {};
        struct MoveTag {};

        template<typename, typename>
        friend class aru::standard::Result;

        Window(HINSTANCE instance, HWND hwnd,
               const aru::standard::String<char16_t>& title,
               std::uint32_t width, std::uint32_t height, AdoptTag);
        Window(const Window& source, CopyTag);
        Window(Window&& source, MoveTag) noexcept;

        static bool registerClass(HINSTANCE instance) noexcept;
        static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

        void release() noexcept;

        HINSTANCE instance = nullptr;
        HWND hwnd = nullptr;
        aru::standard::String<char16_t> title = aru::standard::String<char16_t>::from(u"");
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        bool open = false;
};

aru::window::Window::Window(const Window& other) :
    Window(other, CopyTag{})
{}

aru::window::Window::Window(Window&& other) noexcept :
    Window(std::move(other), MoveTag{})
{}

aru::window::Window::ResultType aru::window::Window::create(const char* title, std::uint32_t width, std::uint32_t height){
    if (title == nullptr) {
        return ResultType(aru::standard::errorInPlace, Error::WindowCreationFailed);
    }
    const auto utf8 = aru::standard::String<char8_t>::from(reinterpret_cast<const char8_t*>(title));
    const auto utf16 = aru::standard::String<char16_t>::from(utf8);
    return create(utf16.stringData(), width, height);
}

aru::window::Window::ResultType aru::window::Window::create(const char16_t* title, std::uint32_t width, std::uint32_t height){
    if (title == nullptr) {
        return ResultType(aru::standard::errorInPlace, Error::WindowCreationFailed);
    }

    HINSTANCE moduleInstance = GetModuleHandleW(nullptr);
    if (moduleInstance == nullptr || !registerClass(moduleInstance)) {
        return ResultType(aru::standard::errorInPlace, Error::ClassRegistrationFailed);
    }

    const auto windowTitle = aru::standard::String<char16_t>::from(title);
    HWND handle = CreateWindowExW(
        0,
        L"ArunimestaWindowClass",
        reinterpret_cast<LPCWSTR>(windowTitle.stringData()),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        static_cast<int>(width),
        static_cast<int>(height),
        nullptr,
        nullptr,
        moduleInstance,
        nullptr
    );

    if (handle == nullptr) {
        return ResultType(aru::standard::errorInPlace, Error::WindowCreationFailed);
    }

    ShowWindow(handle, SW_SHOW);
    UpdateWindow(handle);

    return ResultType(
        std::in_place,
        moduleInstance,
        handle,
        windowTitle,
        width,
        height,
        AdoptTag{}
    );
}

aru::window::Window::ResultType aru::window::Window::createFromWindow(const Window& source){
    return create(source.title.stringData(), source.width, source.height);
}

aru::window::Window& aru::window::Window::operator=(const Window& source){
    if (this == &source) {
        return *this;
    }

    this->release();

    this->instance = source.instance;
    this->title = aru::standard::String<char16_t>::from(source.title);
    this->width = source.width;
    this->height = source.height;
    this->open = false;  // salinan tidak memiliki HWND

    return *this;
}

aru::window::Window& aru::window::Window::operator=(Window&& source) noexcept {
    if (this != &source) {
        this->release();

        this->instance = std::exchange(source.instance, nullptr);
        this->hwnd = std::exchange(source.hwnd, nullptr);
        this->title = source.title.move();
        this->width = std::exchange(source.width, std::uint32_t{0});
        this->height = std::exchange(source.height, std::uint32_t{0});
        this->open = std::exchange(source.open, false);
        // source sekarang kosong.
    }
    return *this;
}

aru::window::Window aru::window::Window::copy() const{
    return Window(*this, CopyTag{});
}

aru::window::Window aru::window::Window::move() noexcept{
    return Window(std::move(*this), MoveTag{});
}

void aru::window::Window::event_loop(){
    if (!this->open) {
        return;
    }

    MSG message = {};
    BOOL status = 1;
    while (this->open && (status = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    // GetMessageW mengembalikan 0 untuk WM_QUIT (dipost oleh windowProc
    // saat WM_DESTROY) atau -1 saat error. Keduanya mengakhiri loop utama.
    if (status <= 0) {
        this->open = false;
    }
}

aru::window::Window::~Window(){
    this->release();
}

HWND aru::window::Window::handle() const noexcept{
    return this->hwnd;
}

const aru::standard::String<char16_t>& aru::window::Window::windowTitle() const noexcept{
    return this->title;
}

std::uint32_t aru::window::Window::windowWidth() const noexcept{
    return this->width;
}

std::uint32_t aru::window::Window::windowHeight() const noexcept{
    return this->height;
}

bool aru::window::Window::empty() const noexcept{
    return this->hwnd == nullptr;
}

bool aru::window::Window::isOpen() const noexcept{
    return this->open;
}

bool aru::window::Window::shouldClose() const noexcept{
    return !this->open;
}

aru::window::Window::Window(HINSTANCE instance, HWND hwnd,
    const aru::standard::String<char16_t>& title,
    std::uint32_t width, std::uint32_t height, AdoptTag) :
    instance(instance),
    hwnd(hwnd),
    title(aru::standard::String<char16_t>::from(title)),
    width(width),
    height(height),
    open(true)
{}

aru::window::Window::Window(const Window& source, CopyTag) :
    instance(source.instance),
    hwnd(nullptr),
    title(aru::standard::String<char16_t>::from(source.title)),
    width(source.width),
    height(source.height),
    open(false)
{}

aru::window::Window::Window(Window&& source, MoveTag) noexcept :
    instance(std::exchange(source.instance, nullptr)),
    hwnd(std::exchange(source.hwnd, nullptr)),
    title(source.title.move()),
    width(std::exchange(source.width, std::uint32_t{0})),
    height(std::exchange(source.height, std::uint32_t{0})),
    open(std::exchange(source.open, false))
{}

bool aru::window::Window::registerClass(HINSTANCE instance) noexcept{
    static bool registered = false;
    if (registered) {
        return true;
    }

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = &Window::windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"ArunimestaWindowClass";
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (RegisterClassW(&windowClass) == 0) {
        const DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    registered = true;
    return true;
}

LRESULT CALLBACK aru::window::Window::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept{
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

void aru::window::Window::release() noexcept{
    // Data pointer / OS handle: cek dulu sebelum "hapus".
    if (this->hwnd != nullptr) {
        DestroyWindow(this->hwnd);
        this->hwnd = nullptr;
    }

    // String members RAII: dikosongkan saat tidak empty, sehingga
    // String destructor yang membebaskan buffer/internal data.
    if (!this->title.empty()) {
        this->title = aru::standard::String<char16_t>::from(u"");
    }

    this->instance = nullptr;
    this->width = 0;
    this->height = 0;
    this->open = false;
}
