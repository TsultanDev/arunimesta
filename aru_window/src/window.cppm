module;
#include <Windows.h>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>

export module aru.window:window;
export import :error;

export namespace aru::window {
    class Window {
        public:
            using ResultType = std::expected<Window, Error>;

            // Factory pembuat OS window baru. title narrow adalah UTF-8.
            static ResultType create(const char* title, std::uint32_t width, std::uint32_t height){
                if (title == nullptr) {
                    return std::unexpected(Error::WindowCreationFailed);
                }
                const std::wstring wide = utf8ToWide(title);
                return create(reinterpret_cast<const char16_t*>(wide.c_str()), width, height);
            }

            static ResultType create(const char16_t* title, std::uint32_t width, std::uint32_t height){
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

                return Window(moduleInstance, handle, windowTitle, width, height);
            }

            // Membuat OS window BARU memakai konfigurasi sumber.
            // Ini bukan copy objek: HWND baru dibuat melalui WinAPI.
            static ResultType createFromWindow(const Window& source){
                return create(reinterpret_cast<const char16_t*>(source.title.c_str()), source.width, source.height);
            }

            // HWND adalah raw handle: salinan tidak diizinkan.
            Window(const Window&) = delete;
            Window& operator=(const Window&) = delete;

            // Move mencuri handle dan mengosongkan sumber.
            Window(Window&& other) noexcept :
                instance(std::exchange(other.instance, nullptr)),
                hwnd(std::exchange(other.hwnd, nullptr)),
                title(std::move(other.title)),
                width(std::exchange(other.width, std::uint32_t{0})),
                height(std::exchange(other.height, std::uint32_t{0})),
                open(std::exchange(other.open, false))
            {}

            Window& operator=(Window&& other) noexcept {
                if (this != &other) {
                    this->release();

                    this->instance = std::exchange(other.instance, nullptr);
                    this->hwnd = std::exchange(other.hwnd, nullptr);
                    this->title = std::move(other.title);
                    this->width = std::exchange(other.width, std::uint32_t{0});
                    this->height = std::exchange(other.height, std::uint32_t{0});
                    this->open = std::exchange(other.open, false);
                }
                return *this;
            }

            ~Window(){ this->release(); }

            void event_loop(){
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

            // Non-blocking variant for render loops: drains pending messages once.
            void pollEvents(){
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

            HWND handle() const noexcept { return this->hwnd; }

            const std::wstring& windowTitle() const noexcept { return this->title; }

            std::uint32_t windowWidth() const noexcept { return this->width; }

            std::uint32_t windowHeight() const noexcept { return this->height; }

            bool empty() const noexcept { return this->hwnd == nullptr; }

            // Penanda untuk loop utama: true sampai WM_QUIT diterima event_loop().
            bool isOpen() const noexcept { return this->open; }

            bool shouldClose() const noexcept { return !this->open; }

        private:
            Window(HINSTANCE instance, HWND hwnd,
                   const std::wstring& title,
                   std::uint32_t width, std::uint32_t height) :
                instance(instance),
                hwnd(hwnd),
                title(title),
                width(width),
                height(height),
                open(true)
            {}

            static std::wstring utf8ToWide(const char* text){
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

            static bool registerClass(HINSTANCE instance) noexcept{
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

            static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept{
                switch (message) {
                    case WM_DESTROY:
                        PostQuitMessage(0);
                        return 0;
                    default:
                        return DefWindowProcW(hwnd, message, wParam, lParam);
                }
            }

            void release() noexcept{
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

            HINSTANCE instance = nullptr;
            HWND hwnd = nullptr;
            std::wstring title;
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            bool open = false;
    };
}
