module;

#include <renaru/vulkan.hpp>

#include <expected>
#include <print>

#include <cstdint>
#include <utility>

export module renaru:adapter;

import :instance;
import :surface;

export namespace renaru {
    class Device;

    class Adapter {
        public:
            using ResultType = std::expected<Adapter, Error>;

            // Didefinisikan di :device (butuh tipe Device yang lengkap).
            std::expected<Device, Error> createDevice(const Adapter& adapter) const;

            // Handle fisik/surface dipinjam: salinan tidak diizinkan.
            Adapter(const Adapter&) = delete;
            Adapter& operator=(const Adapter&) = delete;

            Adapter(Adapter&& other) noexcept :
                mInstance(std::exchange(other.mInstance, vk::Instance{})),
                mPhysicalDevice(std::exchange(other.mPhysicalDevice, vk::PhysicalDevice{})),
                mSurface(std::exchange(other.mSurface, vk::SurfaceKHR{})),
                mQueueFamily(std::exchange(other.mQueueFamily, std::uint32_t{0}))
            {}

            Adapter& operator=(Adapter&& other) noexcept{
                if (this != &other){
                    this->mInstance = std::exchange(other.mInstance, vk::Instance{});
                    this->mPhysicalDevice = std::exchange(other.mPhysicalDevice, vk::PhysicalDevice{});
                    this->mSurface = std::exchange(other.mSurface, vk::SurfaceKHR{});
                    this->mQueueFamily = std::exchange(other.mQueueFamily, std::uint32_t{0});
                }
                return *this;
            }

            ~Adapter() = default;

            vk::PhysicalDevice handle() const noexcept { return this->mPhysicalDevice; }

            vk::Instance instanceHandle() const noexcept { return this->mInstance; }

            vk::SurfaceKHR surfaceHandle() const noexcept { return this->mSurface; }

            std::uint32_t queueFamilyIndex() const noexcept { return this->mQueueFamily; }

            bool empty() const noexcept { return static_cast<VkPhysicalDevice>(this->mPhysicalDevice) == nullptr; }

        private:
            friend class Instance;

            Adapter(vk::Instance instance, vk::PhysicalDevice physicalDevice,
                    vk::SurfaceKHR surface, std::uint32_t queueFamilyIndex) noexcept :
                mInstance(instance),
                mPhysicalDevice(physicalDevice),
                mSurface(surface),
                mQueueFamily(queueFamilyIndex)
            {}

            vk::Instance mInstance;
            vk::PhysicalDevice mPhysicalDevice;
            vk::SurfaceKHR mSurface;
            std::uint32_t mQueueFamily;
    };
}

// Definisi lintas partisi: Instance didefinisikan di :instance, tetapi
// pemilihan adapter membutuhkan tipe lengkap Surface dari partisi ini.
std::expected<renaru::Adapter, renaru::Error> renaru::Instance::requestAdapter(const renaru::Surface& surface){
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
                return Adapter(this->mInstance, physicalDevice, surface.handle(), index);
            }
        }
    }

    return std::unexpected(renaru::Error::AdapterNotFound);
}
