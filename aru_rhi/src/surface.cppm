module;

#include <aru_rhi/vulkan.hpp>

#include <expected>
#include <print>
#include <Windows.h>

#include <utility>

export module aru.rhi:surface;

import aru.window;
import :instance;

export namespace aru::rhi {
    class Surface {
        public:
            using ResultType = std::expected<Surface, Error>;

            // vk::SurfaceKHR adalah raw handle: salinan tidak diizinkan.
            Surface(const Surface&) = delete;
            Surface& operator=(const Surface&) = delete;

            Surface(Surface&& other) noexcept :
                mInstance(std::exchange(other.mInstance, vk::Instance{})),
                mSurface(std::exchange(other.mSurface, vk::SurfaceKHR{}))
            {}

            Surface& operator=(Surface&& other) noexcept{
                if (this != &other){
                    this->release();
                    this->mInstance = std::exchange(other.mInstance, vk::Instance{});
                    this->mSurface = std::exchange(other.mSurface, vk::SurfaceKHR{});
                }
                return *this;
            }

            ~Surface(){ this->release(); }

            vk::SurfaceKHR handle() const noexcept { return this->mSurface; }

            vk::Instance instanceHandle() const noexcept { return this->mInstance; }

            bool empty() const noexcept { return static_cast<VkSurfaceKHR>(this->mSurface) == nullptr; }

        private:
            // Instance::createSurface() adalah satu-satunya jalan pembuatan.
            friend class Instance;

            Surface(vk::Instance instance, vk::SurfaceKHR surface) noexcept :
                mInstance(instance),
                mSurface(surface)
            {}

            void release() noexcept{
                if (static_cast<VkSurfaceKHR>(this->mSurface) != nullptr){
                    this->mInstance.destroySurfaceKHR(this->mSurface, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
                    this->mSurface = vk::SurfaceKHR{};
                }
                this->mInstance = vk::Instance{};
            }

            vk::Instance mInstance;
            vk::SurfaceKHR mSurface;
    };
}

// Definisi lintas partisi: Instance didefinisikan di :instance, tetapi
// pembuatannya membutuhkan tipe lengkap Surface dari partisi ini.
aru::rhi::Surface::ResultType aru::rhi::Instance::createSurface(const aru::window::Window& window){
    if (window.empty()){
        return std::unexpected(aru::rhi::Error::SurfaceCreationFailed);
    }

    vk::Win32SurfaceCreateInfoKHR info;
    info.setHinstance(GetModuleHandleW(nullptr));
    info.setHwnd(window.handle());


    vk::SurfaceKHR created;
    try{
        created = this->mInstance.createWin32SurfaceKHR(info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(aru::rhi::Error::SurfaceCreationFailed);
    }

    std::println("Aru RHI: Surface(created!!)");

    return Surface(this->mInstance, created);
}
