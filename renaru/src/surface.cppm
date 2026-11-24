module;

#include <renaru/vulkan.h>

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
        public:
            using ResultType = std::expected<Surface, Error>;

            // VkSurfaceKHR adalah raw handle: salinan tidak diizinkan.
            Surface(const Surface&) = delete;
            Surface& operator=(const Surface&) = delete;

            Surface(Surface&& sother) noexcept :
                sinstance(std::exchange(sother.sinstance, nullptr)),
                ssurface(std::exchange(sother.ssurface, nullptr))
            {}

            Surface& operator=(Surface&& sother) noexcept{
                if (this != &sother){
                    release();
                    sinstance = std::exchange(sother.sinstance, nullptr);
                    ssurface = std::exchange(sother.ssurface, nullptr);
                }
                return *this;
            }

            ~Surface(){ release(); }

            VkSurfaceKHR handle() const noexcept { return ssurface; }

            VkInstance instanceHandle() const noexcept { return sinstance; }

            bool empty() const noexcept { return ssurface == nullptr; }

        private:
            // Instance::createSurface() adalah satu-satunya jalan pembuatan.
            friend class Instance;

            Surface(VkInstance sinstance, VkSurfaceKHR ssurface) noexcept :
                sinstance(sinstance),
                ssurface(ssurface)
            {}

            void release() noexcept{
                if (ssurface != nullptr && sinstance != nullptr){
                    PFN_vkDestroySurfaceKHR pfnDestroySurfaceKHR =
                        reinterpret_cast<PFN_vkDestroySurfaceKHR>(
                            entry().resolveInstance(sinstance, "vkDestroySurfaceKHR"));
                    if (pfnDestroySurfaceKHR != nullptr){
                        pfnDestroySurfaceKHR(sinstance, ssurface, nullptr);
                    }
                    ssurface = nullptr;
                }
                sinstance = nullptr;
            }

            VkInstance sinstance = nullptr;
            VkSurfaceKHR ssurface = nullptr;
    };
}

// Definisi lintas partisi: Instance didefinisikan di :instance, tetapi
// pembuatannya membutuhkan tipe lengkap Surface dari partisi ini.
renaru::Surface::ResultType renaru::Instance::createSurface(const winaru::Window& swindow){
    if (swindow.empty()){
        return std::unexpected(renaru::Error::SurfaceCreationFailed);
    }

    VkWin32SurfaceCreateInfoKHR sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sinfo.hinstance = GetModuleHandleW(nullptr);
    sinfo.hwnd = swindow.handle();

    PFN_vkCreateWin32SurfaceKHR pfnCreateWin32SurfaceKHR =
        reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
            entry().resolveInstance(sinstance, "vkCreateWin32SurfaceKHR"));
    if (pfnCreateWin32SurfaceKHR == nullptr){
        return std::unexpected(renaru::Error::SurfaceCreationFailed);
    }

    VkSurfaceKHR screated = nullptr;
    if (pfnCreateWin32SurfaceKHR(sinstance, &sinfo, nullptr, &screated) != VK_SUCCESS ||
        screated == nullptr){
        return std::unexpected(renaru::Error::SurfaceCreationFailed);
    }

    std::println("Aru RHI: Surface(created!!)");

    return Surface(sinstance, screated);
}
