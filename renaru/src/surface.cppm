module;

#include <vulkan/vulkan.h>

#include <expected>
#include <print>
#include <Windows.h>
#include <utility>

export module renaru:surface;

import winaru;
import :entry;
import :instance;

export namespace renaru {
    class Surface {
        private:
            using ResultType = std::expected<Surface, Error>;

            Entry& sEntry;
            VkInstance sInstance = nullptr;
            VkSurfaceKHR sSurface = nullptr;

            Surface(Entry& sEntry, VkInstance sInstance, VkSurfaceKHR sSurface) noexcept :
                sEntry(sEntry),
                sInstance(sInstance),
                sSurface(sSurface)
            {}

            void sDestroy() noexcept{
                if (sSurface != nullptr && sInstance != nullptr){
                    PFN_vkDestroySurfaceKHR pfnDestroySurfaceKHR =
                        reinterpret_cast<PFN_vkDestroySurfaceKHR>(
                            sEntry.resolveInstance(sInstance, "vkDestroySurfaceKHR"));
                    if (pfnDestroySurfaceKHR != nullptr){
                        pfnDestroySurfaceKHR(sInstance, sSurface, nullptr);
                    }
                    sSurface = nullptr;
                }
                sInstance = nullptr;
            }

            // Hanya partisi internal yang boleh mengambil data mentah Surface.
            friend class Instance;
            friend class Adapter;

        public:
            ~Surface(){ sDestroy(); }

            Surface(const Surface&) = delete;
            Surface& operator=(const Surface&) = delete;

            Surface(Surface&& sOther) noexcept :
                sEntry(sOther.sEntry),
                sInstance(std::exchange(sOther.sInstance, nullptr)),
                sSurface(std::exchange(sOther.sSurface, nullptr))
            {}

            Surface& operator=(Surface&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    // sEntry tidak di-assign: objek Entry tetap yang dipakai surface ini.
                    sInstance = std::exchange(sOther.sInstance, nullptr);
                    sSurface = std::exchange(sOther.sSurface, nullptr);
                }
                return *this;
            }
    };
}

// Instance::createSurface() didefinisikan di sini karena butuh tipe lengkap Surface.
std::expected<renaru::Surface, renaru::Error>
renaru::Instance::createSurface(const winaru::Window& sWindow){
    if (sWindow.empty()){
        return std::unexpected(renaru::Error::eSurfaceCreationFailed);
    }

    VkWin32SurfaceCreateInfoKHR sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sInfo.hinstance = GetModuleHandleW(nullptr);
    sInfo.hwnd = sWindow.handle();

    PFN_vkCreateWin32SurfaceKHR pfnCreateWin32SurfaceKHR =
        reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
            sEntry.resolveInstance(sInstance, "vkCreateWin32SurfaceKHR"));
    if (pfnCreateWin32SurfaceKHR == nullptr){
        return std::unexpected(renaru::Error::eSurfaceCreationFailed);
    }

    VkSurfaceKHR sCreated = nullptr;
    if (pfnCreateWin32SurfaceKHR(sInstance, &sInfo, nullptr, &sCreated) != VK_SUCCESS ||
        sCreated == nullptr){
        return std::unexpected(renaru::Error::eSurfaceCreationFailed);
    }

    std::println("Aru RHI: Surface(created!!)");

    return Surface(sEntry, sInstance, sCreated);
}
