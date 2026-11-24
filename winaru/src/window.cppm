module;
#include <Windows.h>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>

export module winaru:window;
export import :error;

export namespace winaru {
    class Window {
        private:
            using ResultType = std::expected<Window, Error>;

            HINSTANCE sInstance = nullptr;
            HWND sHwnd = nullptr;
            std::wstring sTitle{};
            std::uint32_t sWidth = 0;
            std::uint32_t sHeight = 0;
            bool sOpen = false;

            Window(HINSTANCE sHInstance, HWND sHwnd,
                   const std::wstring& sTitle,
                   std::uint32_t sWidth, std::uint32_t sHeight) noexcept :
                sInstance(sHInstance),
                sHwnd(sHwnd),
                sTitle(sTitle),
                sWidth(sWidth),
                sHeight(sHeight),
                sOpen(true)
            {}

            // Judul narrow adalah UTF-8; dikonversi dulu ke UTF-16.
            static ResultType sCreate(const char* sTitle, std::uint32_t sWidth,
                                      std::uint32_t sHeight){
                if (sTitle == nullptr){
                    return std::unexpected(Error::WindowCreationFailed);
                }
                const std::wstring sWide = sUtf8ToWide(sTitle);
                return sCreate(reinterpret_cast<const char16_t*>(sWide.c_str()), sWidth, sHeight);
            }

            static ResultType sCreate(const char16_t* sTitle, std::uint32_t sWidth,
                                      std::uint32_t sHeight){
                if (sTitle == nullptr){
                    return std::unexpected(Error::WindowCreationFailed);
                }

                HINSTANCE sInstance = GetModuleHandleW(nullptr);
                if (sInstance == nullptr || !sRegisterClass(sInstance)){
                    return std::unexpected(Error::ClassRegistrationFailed);
                }

                // wchar_t berukuran 16-bit di Windows, jadi char16_t bisa dipetakan langsung.
                const std::wstring sWindowTitle(reinterpret_cast<const wchar_t*>(sTitle));
                HWND sHwnd = CreateWindowExW(
                    0,
                    L"ArunimestaWindowClass",
                    sWindowTitle.c_str(),
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    static_cast<int>(sWidth),
                    static_cast<int>(sHeight),
                    nullptr,
                    nullptr,
                    sInstance,
                    nullptr
                );

                if (sHwnd == nullptr){
                    return std::unexpected(Error::WindowCreationFailed);
                }

                ShowWindow(sHwnd, SW_SHOW);
                UpdateWindow(sHwnd);

                return Window(sInstance, sHwnd, sWindowTitle, sWidth, sHeight);
            }

            // Bersihkan handle OS dan state sebelum object mati atau di-move-assign.
            void sDestroy() noexcept{
                if (sHwnd != nullptr){
                    DestroyWindow(sHwnd);
                    sHwnd = nullptr;
                }
                sTitle.clear();
                sInstance = nullptr;
                sWidth = 0;
                sHeight = 0;
                sOpen = false;
            }

            static std::wstring sUtf8ToWide(const char* sText){
                if (sText == nullptr){
                    return {};
                }
                const int sNeeded = MultiByteToWideChar(CP_UTF8, 0, sText, -1, nullptr, 0);
                if (sNeeded <= 0){
                    return {};
                }
                std::wstring sResult(static_cast<std::size_t>(sNeeded), L'\0');
                MultiByteToWideChar(CP_UTF8, 0, sText, -1, sResult.data(), sNeeded);
                sResult.resize(static_cast<std::size_t>(sNeeded - 1));
                return sResult;
            }

            static bool sRegisterClass(HINSTANCE sInstance) noexcept{
                static bool sRegistered = false;
                if (sRegistered){
                    return true;
                }

                WNDCLASSW sWindowClass = {};
                sWindowClass.lpfnWndProc = &Window::sWindowProc;
                sWindowClass.hInstance = sInstance;
                sWindowClass.lpszClassName = L"ArunimestaWindowClass";
                sWindowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
                sWindowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

                if (RegisterClassW(&sWindowClass) == 0){
                    const DWORD sError = GetLastError();
                    if (sError != ERROR_CLASS_ALREADY_EXISTS){
                        return false;
                    }
                }

                sRegistered = true;
                return true;
            }

            static LRESULT CALLBACK sWindowProc(HWND sHwnd, UINT sMessage,
                                                WPARAM sWParam, LPARAM sLParam) noexcept{
                switch (sMessage){
                    case WM_DESTROY:
                        PostQuitMessage(0);
                        return 0;
                    default:
                        return DefWindowProcW(sHwnd, sMessage, sWParam, sLParam);
                }
            }

        public:
            // Factory pembuat OS window baru.
            static ResultType create(const char* sTitle, std::uint32_t sWidth,
                                     std::uint32_t sHeight){
                return sCreate(sTitle, sWidth, sHeight);
            }

            static ResultType create(const char16_t* sTitle, std::uint32_t sWidth,
                                     std::uint32_t sHeight){
                return sCreate(sTitle, sWidth, sHeight);
            }

            // Membuat OS window BARU memakai konfigurasi sumber, bukan copy objek.
            static ResultType createFromWindow(const Window& sOther){
                return sCreate(reinterpret_cast<const char16_t*>(sOther.sTitle.c_str()),
                               sOther.sWidth, sOther.sHeight);
            }

            ~Window(){ sDestroy(); }

            Window(const Window&) = delete;
            Window& operator=(const Window&) = delete;

            // Move mencuri handle dan mengosongkan sumber.
            Window(Window&& sOther) noexcept :
                sInstance(std::exchange(sOther.sInstance, nullptr)),
                sHwnd(std::exchange(sOther.sHwnd, nullptr)),
                sTitle(std::move(sOther.sTitle)),
                sWidth(std::exchange(sOther.sWidth, std::uint32_t{0})),
                sHeight(std::exchange(sOther.sHeight, std::uint32_t{0})),
                sOpen(std::exchange(sOther.sOpen, false))
            {}

            Window& operator=(Window&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sInstance = std::exchange(sOther.sInstance, nullptr);
                    sHwnd = std::exchange(sOther.sHwnd, nullptr);
                    sTitle = std::move(sOther.sTitle);
                    sWidth = std::exchange(sOther.sWidth, std::uint32_t{0});
                    sHeight = std::exchange(sOther.sHeight, std::uint32_t{0});
                    sOpen = std::exchange(sOther.sOpen, false);
                }
                return *this;
            }

            void event_loop(){
                if (!sOpen){
                    return;
                }

                MSG sMessage = {};
                BOOL sStatus = 1;
                while (sOpen && (sStatus = GetMessageW(&sMessage, nullptr, 0, 0)) > 0){
                    TranslateMessage(&sMessage);
                    DispatchMessageW(&sMessage);
                }

                // GetMessageW mengembalikan 0 untuk WM_QUIT atau -1 saat error.
                if (sStatus <= 0){
                    sOpen = false;
                }
            }

            // Non-blocking variant for render loops: drains pending messages once.
            void pollEvents(){
                if (!sOpen){
                    return;
                }

                MSG sMessage = {};
                while (PeekMessageW(&sMessage, nullptr, 0, 0, PM_REMOVE)){
                    if (sMessage.message == WM_QUIT){
                        sOpen = false;
                        break;
                    }
                    TranslateMessage(&sMessage);
                    DispatchMessageW(&sMessage);
                }
            }

            HWND handle() const noexcept{ return sHwnd; }

            const std::wstring& windowTitle() const noexcept{ return sTitle; }

            std::uint32_t windowWidth() const noexcept{ return sWidth; }

            std::uint32_t windowHeight() const noexcept{ return sHeight; }

            bool empty() const noexcept{ return sHwnd == nullptr; }

            // Penanda untuk loop utama: true sampai WM_QUIT diterima event_loop().
            bool isOpen() const noexcept{ return sOpen; }

            bool shouldClose() const noexcept{ return !sOpen; }
    };
}
