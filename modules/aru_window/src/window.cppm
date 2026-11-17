module;
#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <memory>
#include <minwindef.h>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <winbase.h>

export module aru.window:window;

static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static std::wstring toWide(std::string_view text);

export namespace aru {
    namespace window {
        enum class Error;
        class Window;
    }
}

static std::wstring toWide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    if (text.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
        return {};
    }
    const int length = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0
    );
    if (length <= 0) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(length), L'\0');
    const int converted = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length
    );
    if (converted <= 0) {
        return {};
    }
    return wide;
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

enum class aru::window::Error{
    None,
    ModuleHandleFailed,
    ClassRegistrationFailed,
    ZeroSize,
    CreationFailed
};

class aru::window::Window {
    public:
        static std::expected<Window, Error> create_window(std::string title, uint32_t width, uint32_t height);
        bool event_loop();

        Window(const Window&);
        Window& operator=(const Window&);

        Window(Window&&) noexcept = default;
        Window& operator=(Window&&) noexcept = default;

        void* operator new(size_t) = delete;
        void* operator new[](size_t) = delete;

        Window* operator->() = delete;

        Window() = default;
        ~Window() = default;
    private:

        Error createInstanceHandle();
        Error createWindowHandle();

        struct Data{
          std::string title = "Arunimesta";
          uint32_t width = 800;
          uint32_t height = 600;
          HINSTANCE instance = nullptr;
          HWND hwnd = nullptr;
        };
        std::unique_ptr<Data> data = nullptr;
        Data* pData = nullptr;
};

std::expected<aru::window::Window, aru::window::Error> aru::window::Window::create_window(std::string title, uint32_t width, uint32_t height){
    aru::window::Window window{};
    window.data = std::make_unique<Data>();
    auto createInstanceHandle = window.createInstanceHandle();
    if(createInstanceHandle != aru::window::Error::None){
        return std::unexpected<Error>(createInstanceHandle);
    };
    auto createWindowHandle = window.createWindowHandle();
    if(createWindowHandle != aru::window::Error::None){
        return std::unexpected<Error>(createWindowHandle);
    };

    return window;
}

aru::window::Window::Window(const Window &other){
    this->data = std::move(other.data);
}
aru::window::Window& aru::window::Window::operator=(const Window& other){
    this->pData = other.data.get() ? other.data.get() : nullptr;
    return
}

aru::window::Error aru::window::Window::createInstanceHandle(){
    auto instance = GetModuleHandleW(nullptr);
    if (!this->data->instance) {
        return Error::ModuleHandleFailed;
    }
    this->data->instance = instance;

    return Error::None;
}
aru::window::Error aru::window::Window::createWindowHandle(){
    auto& data = *this->data.get();

    constexpr wchar_t CLASS_NAME[] = L"ArunimestaClass";

    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.lpszClassName = CLASS_NAME;
    window_class.hInstance = this->data->instance;
    window_class.lpfnWndProc = windowProc;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if (!RegisterClassExW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return aru::window::Error::ClassRegistrationFailed;
    }

    const std::wstring wideTitle = toWide(data.title);
    if (!this->data->title.empty() && wideTitle.empty()) {
        return aru::window::Error::CreationFailed;
    }

    auto hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        wideTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        static_cast<int>(data.width),
        static_cast<int>(data.height),
        nullptr,
        nullptr,
        this->data->instance,
        nullptr
    );
    if (hwnd == nullptr) {
        return aru::window::Error::CreationFailed;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);

    this->data->hwnd = hwnd;

    return Error::None;
}

bool aru::window::Window::event_loop() {
    if (!this->data->hwnd) {
        return false;
    }

    MSG msg = {};
    bool hadMessage = false;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        hadMessage = true;
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (!hadMessage) {
        WaitMessage();
    }
    return true;
}
