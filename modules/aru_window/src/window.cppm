module;
#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <utility>

export module aru.window:window;

static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static std::wstring toWide(std::string_view text);

namespace aru {
    namespace window {
        export enum class WindowError {
            ModuleHandleFailed,
            ClassRegistrationFailed,
            InvalidArguments,
            CreationFailed
        };

        struct Window_T;
        export class Window;
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

struct aru::window::Window_T {
    HINSTANCE instance = nullptr;
    HWND hwnd = nullptr;

    ~Window_T() {
        if (hwnd != nullptr) {
            DestroyWindow(hwnd);
        }
    }
};

class aru::window::Window {
    public:
        static std::expected<Window, WindowError> create_window(std::string title, uint32_t width, uint32_t height);
        bool event_loop();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&&) noexcept = default;
        Window& operator=(Window&&) noexcept = default;

        void* operator new(size_t) = delete;
        void* operator new[](size_t) = delete;

    private:
        Window(std::unique_ptr<Window_T> window);
        std::unique_ptr<Window_T> window;
};

aru::window::Window::Window(std::unique_ptr<aru::window::Window_T> window) : window(std::move(window)) {
}

std::expected<aru::window::Window, aru::window::WindowError> aru::window::Window::create_window(std::string title, uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return std::unexpected<WindowError>(WindowError::InvalidArguments);
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!instance) {
        return std::unexpected<WindowError>(WindowError::ModuleHandleFailed);
    }

    constexpr wchar_t CLASS_NAME[] = L"ArunimestaClass";

    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.lpszClassName = CLASS_NAME;
    window_class.hInstance = instance;
    window_class.lpfnWndProc = windowProc;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if (!RegisterClassExW(&window_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return std::unexpected<WindowError>(WindowError::ClassRegistrationFailed);
    }

    const std::wstring wideTitle = toWide(title);
    if (!title.empty() && wideTitle.empty()) {
        return std::unexpected<WindowError>(WindowError::CreationFailed);
    }

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
    if (hwnd == nullptr) {
        return std::unexpected<WindowError>(WindowError::CreationFailed);
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);

    auto impl = std::make_unique<aru::window::Window_T>();
    impl->instance = instance;
    impl->hwnd = hwnd;

    auto window = aru::window::Window(std::move(impl));

    #ifndef NDEBUG
    std::println(stderr, "Aru: Window(created!!), title: {}", title);
    #endif

    return window;
}

bool aru::window::Window::event_loop() {
    if (!this->window) {
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