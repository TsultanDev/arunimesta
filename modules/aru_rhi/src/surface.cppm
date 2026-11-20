module;

#include <aru_rhi/vulkan.hpp>
#include <Windows.h>

#include <utility>

export module aru.rhi:surface;

import aru.standard;
import aru.window;
import :instance;

export namespace aru::rhi {
    class Surface;
}

export namespace aru::rhi {
    class Surface {
        public:
            using ResultType = aru::standard::Result<Surface, Error>;

            Surface(const Surface& other);
            Surface(Surface&& other) noexcept;
            Surface& operator=(const Surface& other);
            Surface& operator=(Surface&& other) noexcept;
            ~Surface();

            Surface copy() const;
            Surface move() noexcept;

            vk::SurfaceKHR handle() const noexcept;
            vk::Instance instanceHandle() const noexcept;
            bool empty() const noexcept;

        private:
            // Instance::createSurface() adalah satu-satunya jalan pembuatan.
            friend class Instance;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Surface(vk::Instance instance, vk::SurfaceKHR surface, InitTag tag) noexcept;
            Surface(const Surface& other, CopyTag tag);
            Surface(Surface&& other, MoveTag tag) noexcept;

            void release() noexcept;

            vk::Instance mInstance;
            vk::SurfaceKHR mSurface;
    };
}

aru::rhi::Surface::ResultType aru::rhi::Instance::createSurface(const aru::window::Window& window){
    if (window.empty()){
        return aru::rhi::Error::SurfaceCreationFailed;
    }

    vk::Win32SurfaceCreateInfoKHR info;
    info.setHinstance(GetModuleHandleW(nullptr));
    info.setHwnd(window.handle());


    vk::SurfaceKHR created;
    try{
        created = this->mInstance.createWin32SurfaceKHR(info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return aru::rhi::Error::SurfaceCreationFailed;
    }

    aru::standard::println("Aru RHI: Surface(created!!)");

    return Surface(this->mInstance, created, Surface::InitTag{});
}

aru::rhi::Surface::Surface(vk::Instance instance, vk::SurfaceKHR surface, InitTag) noexcept :
    mInstance(instance),
    mSurface(surface)
{}

aru::rhi::Surface::Surface(const Surface& other) :
    Surface(other, CopyTag{})
{}

aru::rhi::Surface::Surface(Surface&& other) noexcept :
    Surface(std::move(other), MoveTag{})
{}

aru::rhi::Surface::Surface(const Surface& other, CopyTag) :
    mInstance(other.mInstance),
    mSurface()
{}

aru::rhi::Surface::Surface(Surface&& other, MoveTag) noexcept :
    mInstance(other.mInstance),
    mSurface(std::exchange(other.mSurface, vk::SurfaceKHR{}))
{}

aru::rhi::Surface& aru::rhi::Surface::operator=(const Surface& other){
    if (this != &other){
        Surface fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::Surface& aru::rhi::Surface::operator=(Surface&& other) noexcept{
    if (this != &other){
        this->release();
        this->mInstance = other.mInstance;
        this->mSurface = std::exchange(other.mSurface, vk::SurfaceKHR{});
    }
    return *this;
}

aru::rhi::Surface aru::rhi::Surface::copy() const{
    return Surface(*this, CopyTag{});
}

aru::rhi::Surface aru::rhi::Surface::move() noexcept{
    return Surface(std::move(*this), MoveTag{});
}

vk::SurfaceKHR aru::rhi::Surface::handle() const noexcept{
    return this->mSurface;
}

vk::Instance aru::rhi::Surface::instanceHandle() const noexcept{
    return this->mInstance;
}

bool aru::rhi::Surface::empty() const noexcept{
    return static_cast<VkSurfaceKHR>(this->mSurface) == nullptr;
}

void aru::rhi::Surface::release() noexcept{
    if (static_cast<VkSurfaceKHR>(this->mSurface) != nullptr){
        this->mInstance.destroySurfaceKHR(this->mSurface, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        this->mSurface = vk::SurfaceKHR{};
    }
}

aru::rhi::Surface::~Surface(){
    this->release();
}
