module;
#include <Windows.h>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>

export module aru.window:window;
export import :error;

export namespace aru {
    namespace window {
        class Window;
    }
}

class aru::window::Window{
    public:
        using ResultType = std::expected<Window, Error>;

        // Factory pembuat OS window baru. title narrow adalah UTF-8.
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

        // Non-blocking variant for render loops: drains pending messages once.
        void pollEvents();
        ~Window();

        HWND handle() const noexcept;
        const std::wstring& windowTitle() const noexcept;
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

        Window(HINSTANCE instance, HWND hwnd,
               const std::wstring& title,
               std::uint32_t width, std::uint32_t height, AdoptTag);
        Window(const Window& source, CopyTag);
        Window(Window&& source, MoveTag) noexcept;

        static std::wstring utf8ToWide(const char* text);
        static bool registerClass(HINSTANCE instance) noexcept;
        static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

        void release() noexcept;

        HINSTANCE instance = nullptr;
        HWND hwnd = nullptr;
        std::wstring title;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        bool open = false;
};

std::wstring aru::window::Window::utf8ToWide(const char* text){
    if (text == nullptr) {
        return {};
    }
    const int needed = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (needed <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), needed);
    result.resize(static_cast<std::size_t>(needed - 1));
    return result;
}

aru::window::Window::Window(const Window& other) :
    Window(other, CopyTag{})
{}

aru::window::Window::Window(Window&& other) noexcept :
    Window(std::move(other), MoveTag{})
{}

aru::window::Window::ResultType aru::window::Window::create(const char* title, std::uint32_t width, std::uint32_t height){
    if (title == nullptr) {
        return std::unexpected(Error::WindowCreationFailed);
    }
    const std::wstring wide = utf8ToWide(title);
    return create(reinterpret_cast<const char16_t*>(wide.c_str()), width, height);
}

aru::window::Window::ResultType aru::window::Window::create(const char16_t* title, std::uint32_t width, std::uint32_t height){
    if (title == nullptr) {
        return std::unexpected(Error::WindowCreationFailed);
    }

    HINSTANCE moduleInstance = GetModuleHandleW(nullptr);
    if (moduleInstance == nullptr || !registerClass(moduleInstance)) {
        return std::unexpected(Error::ClassRegistrationFailed);
    }

    // wchar_t berukuran 16-bit di Windows, jadi char16_t bisa dipetakan langsung.
    const std::wstring windowTitle(reinterpret_cast<const wchar_t*>(title));
    HWND handle = CreateWindowExW(
        0,
        L"ArunimestaWindowClass",
        windowTitle.c_str(),
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
        return std::unexpected(Error::WindowCreationFailed);
    }

    ShowWindow(handle, SW_SHOW);
    UpdateWindow(handle);

    return Window(moduleInstance, handle, windowTitle, width, height, AdoptTag{});
}

aru::window::Window::ResultType aru::window::Window::createFromWindow(const Window& source){
    return create(reinterpret_cast<const char16_t*>(source.title.c_str()), source.width, source.height);
}

aru::window::Window& aru::window::Window::operator=(const Window& source){
    if (this == &source) {
        return *this;
    }

    this->release();

    this->instance = source.instance;
    this->title = source.title;
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
        this->title = std::move(source.title);
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

void aru::window::Window::pollEvents(){
    if (!this->open) {
        return;
    }

    MSG message = {};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            this->open = false;
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

aru::window::Window::~Window(){
    this->release();
}

HWND aru::window::Window::handle() const noexcept{
    return this->hwnd;
}

const std::wstring& aru::window::Window::windowTitle() const noexcept{
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
    const std::wstring& title,
    std::uint32_t width, std::uint32_t height, AdoptTag) :
    instance(instance),
    hwnd(hwnd),
    title(title),
    width(width),
    height(height),
    open(true)
{}

aru::window::Window::Window(const Window& source, CopyTag) :
    instance(source.instance),
    hwnd(nullptr),
    title(source.title),
    width(source.width),
    height(source.height),
    open(false)
{}

aru::window::Window::Window(Window&& source, MoveTag) noexcept :
    instance(std::exchange(source.instance, nullptr)),
    hwnd(std::exchange(source.hwnd, nullptr)),
    title(std::move(source.title)),
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

    this->title.clear();
    this->instance = nullptr;
    this->width = 0;
    this->height = 0;
    this->open = false;
}
