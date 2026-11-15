module;
#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <winuser.h>

export module aru.window:window;

static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static std::wstring toWide(std::string_view text);

namespace aru {
    namespace window {
        struct Window_T;
        export class Window;
    }
}

static std::wstring toWide(std::string_view text)
{
    if (text.empty()) {
        return {};
    }

    const int length = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0
    );
    if (length <= 0) {
        return {};
    }

    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length
    );
    return wide;
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
    switch(message){
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

struct aru::window::Window_T{
    HINSTANCE instance = nullptr;
    HWND hwnd = nullptr;
};

class aru::window::Window{
    public:
        static std::expected<Window, bool> create_window(std::string title, uint32_t width, uint32_t height);
        bool event_loop();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&&) noexcept = default;
        Window& operator=(Window&&) noexcept = default;

    private:
        Window(std::unique_ptr<Window_T> window);
        std::unique_ptr<Window_T> window;
};

aru::window::Window::Window(std::unique_ptr<aru::window::Window_T> window) : window(std::move(window)){

}

std::expected<aru::window::Window, bool> aru::window::Window::create_window(std::string title, uint32_t width, uint32_t height){
    HINSTANCE instance = GetModuleHandleW(nullptr);
    if(!instance){
        return std::unexpected<bool>(false);
    }

    constexpr wchar_t CLASS_NAME[] = L"ArunimestaClass";

    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.lpszClassName = CLASS_NAME;
    window_class.hInstance = instance;
    window_class.lpfnWndProc = windowProc;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if(!RegisterClassExW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS){
        return std::unexpected<bool>(false);
    }

    // The public API takes UTF-8; Win32 expects UTF-16.
    const std::wstring wideTitle = toWide(title);

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        wideTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        static_cast<int>(width),
        static_cast<int>(height),
        nullptr,
        nullptr,
        instance,
        nullptr
    );
    if(hwnd == nullptr){
        return std::unexpected<bool>(false);
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);

    auto window = aru::window::Window(std::make_unique<aru::window::Window_T>(aru::window::Window_T{
        .instance = instance,
        .hwnd = hwnd
    }));

    #ifndef NDEBUG
    std::println(stderr, "Aru: Window(created!!), title: {}", title);
    #endif

    return window;
}

bool aru::window::Window::event_loop(){
    MSG msg = {};
    while(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if(msg.message == WM_QUIT){
            return false;
        }
    }
    return true;
}
