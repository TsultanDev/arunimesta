module;

#include <aru_rhi/vulkan.hpp>

#include <expected>
#include <print>

#include <cstdint>
#include <utility>

export module aru.rhi:adapter;

import :instance;
import :surface;

export namespace aru::rhi {
    class Adapter;
}

export namespace aru::rhi {
    class Adapter {
        public:
            using ResultType = std::expected<Adapter, Error>;

            // Didefinisikan di :device (butuh tipe Device yang lengkap).
            std::expected<Device, Error> createDevice(const Adapter& adapter) const;

            Adapter(const Adapter& other);
            Adapter(Adapter&& other) noexcept;
            Adapter& operator=(const Adapter& other);
            Adapter& operator=(Adapter&& other) noexcept;
            ~Adapter() = default;

            Adapter copy() const;
            Adapter move() noexcept;

            vk::PhysicalDevice handle() const noexcept;
            vk::Instance instanceHandle() const noexcept;
            vk::SurfaceKHR surfaceHandle() const noexcept;
            std::uint32_t queueFamilyIndex() const noexcept;
            bool empty() const noexcept;

        private:
            friend class Instance;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Adapter(vk::Instance instance, vk::PhysicalDevice physicalDevice,
                    vk::SurfaceKHR surface, std::uint32_t queueFamilyIndex,
                    InitTag tag) noexcept;
            Adapter(const Adapter& other, CopyTag tag);
            Adapter(Adapter&& other, MoveTag tag) noexcept;

            vk::Instance mInstance;
            vk::PhysicalDevice mPhysicalDevice;
            vk::SurfaceKHR mSurface;
            std::uint32_t mQueueFamily;
    };
}

std::expected<aru::rhi::Adapter, aru::rhi::Error> aru::rhi::Instance::requestAdapter(const aru::rhi::Surface& surface){
    const auto devices = this->mInstance.enumeratePhysicalDevices(VULKAN_HPP_DEFAULT_DISPATCHER);

    for (const vk::PhysicalDevice& physicalDevice : devices){
        const auto families = physicalDevice.getQueueFamilyProperties(VULKAN_HPP_DEFAULT_DISPATCHER);
        for (std::uint32_t index = 0; index < families.size(); ++index){
            if ((static_cast<VkQueueFlags>(families[index].queueFlags) & VK_QUEUE_GRAPHICS_BIT) == 0){
                continue;
            }

            const vk::Bool32 supportsPresent = physicalDevice.getSurfaceSupportKHR(
                index, surface.handle(), VULKAN_HPP_DEFAULT_DISPATCHER
            );
            if (supportsPresent){
                std::println("Aru RHI: Adapter(found!!) family={}", index);
                return Adapter(this->mInstance, physicalDevice, surface.handle(), index, Adapter::InitTag{});
            }
        }
    }

    return std::unexpected(aru::rhi::Error::AdapterNotFound);
}

aru::rhi::Adapter::Adapter(vk::Instance instance, vk::PhysicalDevice physicalDevice,
                           vk::SurfaceKHR surface, std::uint32_t queueFamilyIndex, InitTag) noexcept :
    mInstance(instance),
    mPhysicalDevice(physicalDevice),
    mSurface(surface),
    mQueueFamily(queueFamilyIndex)
{}

aru::rhi::Adapter::Adapter(const Adapter& other) :
    Adapter(other, CopyTag{})
{}

aru::rhi::Adapter::Adapter(Adapter&& other) noexcept :
    Adapter(std::move(other), MoveTag{})
{}

aru::rhi::Adapter::Adapter(const Adapter& other, CopyTag) :
    mInstance(other.mInstance),
    mPhysicalDevice(),
    mSurface(),
    mQueueFamily(0)
{}

aru::rhi::Adapter::Adapter(Adapter&& other, MoveTag) noexcept :
    mInstance(other.mInstance),
    mPhysicalDevice(std::exchange(other.mPhysicalDevice, vk::PhysicalDevice{})),
    mSurface(std::exchange(other.mSurface, vk::SurfaceKHR{})),
    mQueueFamily(std::exchange(other.mQueueFamily, std::uint32_t{0}))
{}

aru::rhi::Adapter& aru::rhi::Adapter::operator=(const Adapter& other){
    if (this != &other){
        Adapter fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::Adapter& aru::rhi::Adapter::operator=(Adapter&& other) noexcept{
    if (this != &other){
        this->mInstance = other.mInstance;
        this->mPhysicalDevice = std::exchange(other.mPhysicalDevice, vk::PhysicalDevice{});
        this->mSurface = std::exchange(other.mSurface, vk::SurfaceKHR{});
        this->mQueueFamily = std::exchange(other.mQueueFamily, std::uint32_t{0});
    }
    return *this;
}

aru::rhi::Adapter aru::rhi::Adapter::copy() const{
    return Adapter(*this, CopyTag{});
}

aru::rhi::Adapter aru::rhi::Adapter::move() noexcept{
    return Adapter(std::move(*this), MoveTag{});
}

vk::PhysicalDevice aru::rhi::Adapter::handle() const noexcept{
    return this->mPhysicalDevice;
}

vk::Instance aru::rhi::Adapter::instanceHandle() const noexcept{
    return this->mInstance;
}

vk::SurfaceKHR aru::rhi::Adapter::surfaceHandle() const noexcept{
    return this->mSurface;
}

std::uint32_t aru::rhi::Adapter::queueFamilyIndex() const noexcept{
    return this->mQueueFamily;
}

bool aru::rhi::Adapter::empty() const noexcept{
    return static_cast<VkPhysicalDevice>(this->mPhysicalDevice) == nullptr;
}
