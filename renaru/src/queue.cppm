module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <print>
#include <utility>
#include <vector>

export module renaru:queue;

import :entry;
import :instance;
import :device;

export namespace renaru {
    class CommandBuffer;

    class Queue {
        private:
            using ResultType = std::expected<Queue, Error>;

            const Device* sOwner = nullptr;
            VkDevice sDevice = nullptr;
            VkQueue sQueue = nullptr;
            QueueFlags sFlags = QueueFlags::None;
            std::uint32_t sFamily = 0;

            Queue(const Device* sOwner, VkDevice sDevice, VkQueue sQueue,
                  QueueFlags sFlags, std::uint32_t sFamilyIndex) noexcept :
                sOwner(sOwner),
                sDevice(sDevice),
                sQueue(sQueue),
                sFlags(sFlags),
                sFamily(sFamilyIndex)
            {}

            friend class Device;

        public:
            ~Queue() = default;

            Queue(const Queue&) = delete;
            Queue& operator=(const Queue&) = delete;

            Queue(Queue&& sOther) noexcept :
                sOwner(std::exchange(sOther.sOwner, nullptr)),
                sDevice(std::exchange(sOther.sDevice, nullptr)),
                sQueue(std::exchange(sOther.sQueue, nullptr)),
                sFlags(std::exchange(sOther.sFlags, QueueFlags::None)),
                sFamily(std::exchange(sOther.sFamily, std::uint32_t{0}))
            {}

            Queue& operator=(Queue&& sOther) noexcept{
                if (this != &sOther){
                    sOwner = std::exchange(sOther.sOwner, nullptr);
                    sDevice = std::exchange(sOther.sDevice, nullptr);
                    sQueue = std::exchange(sOther.sQueue, nullptr);
                    sFlags = std::exchange(sOther.sFlags, QueueFlags::None);
                    sFamily = std::exchange(sOther.sFamily, std::uint32_t{0});
                }
                return *this;
            }

            std::expected<void, Error> submit(const CommandBuffer& sBuffer) const;
            std::expected<void, Error> present() const;
    };
}

std::expected<renaru::Queue, renaru::Error>
renaru::Device::requestQueue(renaru::QueueFlags sFlags) const{
    if (sDevice == nullptr){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }

    PFN_vkGetPhysicalDeviceQueueFamilyProperties pfnGetQueueFamilyProperties =
        reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
            entry().resolveInstance(sInstance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    if (pfnGetQueueFamilyProperties == nullptr){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }

    std::uint32_t sFamilyCount = 0;
    pfnGetQueueFamilyProperties(sPhysicalDevice, &sFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> sFamilies(sFamilyCount);
    pfnGetQueueFamilyProperties(sPhysicalDevice, &sFamilyCount, sFamilies.data());

    if (sQueueFamily >= sFamilies.size()){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }
    const VkQueueFlags sAvailable = sFamilies[sQueueFamily].queueFlags;

    if (hasFlag(sFlags, QueueFlags::Graphics) && (sAvailable & VK_QUEUE_GRAPHICS_BIT) == 0){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }
    if (hasFlag(sFlags, QueueFlags::Compute) && (sAvailable & VK_QUEUE_COMPUTE_BIT) == 0){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }
    if (hasFlag(sFlags, QueueFlags::Transfer) && (sAvailable & VK_QUEUE_TRANSFER_BIT) == 0){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }
    if (hasFlag(sFlags, QueueFlags::Present)){
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfnGetSurfaceSupportKHR =
            reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
                entry().resolveInstance(sInstance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
        VkBool32 sSupportsPresent = VK_FALSE;
        if (pfnGetSurfaceSupportKHR == nullptr){
            return std::unexpected(renaru::Error::QueueRequestFailed);
        }
        pfnGetSurfaceSupportKHR(sPhysicalDevice, sQueueFamily, sSurface, &sSupportsPresent);
        if (sSupportsPresent != VK_TRUE){
            return std::unexpected(renaru::Error::QueueRequestFailed);
        }
    }

    PFN_vkGetDeviceQueue pfnGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(
        entry().resolveDevice(sDevice, "vkGetDeviceQueue"));
    if (pfnGetDeviceQueue == nullptr){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }

    VkQueue sQueue = nullptr;
    pfnGetDeviceQueue(sDevice, sQueueFamily, 0, &sQueue);
    if (sQueue == nullptr){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }

    std::println("Aru RHI: Queue(requested!!) family={}", sQueueFamily);

    return Queue(this, sDevice, sQueue, sFlags, sQueueFamily);
}
