module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <print>
#include <utility>
#include <vector>

export module renaru:adapter;

import :entry;
import :instance;
import :surface;

export namespace renaru {
    class Device;

    class Adapter {
        private:
            using ResultType = std::expected<Adapter, Error>;

            Entry& sEntry;
            VkInstance sInstance = nullptr;
            VkPhysicalDevice sPhysicalDevice = nullptr;
            VkSurfaceKHR sSurface = nullptr;
            std::uint32_t sQueueFamily = 0;

            Adapter(Entry& sEntry, VkInstance sInstance, VkPhysicalDevice sPhysicalDevice,
                    VkSurfaceKHR sSurface, std::uint32_t sQueueFamilyIndex) noexcept :
                sEntry(sEntry),
                sInstance(sInstance),
                sPhysicalDevice(sPhysicalDevice),
                sSurface(sSurface),
                sQueueFamily(sQueueFamilyIndex)
            {}

            // Partisi lain hanya mengambil data mentah melalui friend.
            friend class Instance;
            friend class Device;

        public:
            ~Adapter() = default;

            Adapter(const Adapter&) = delete;
            Adapter& operator=(const Adapter&) = delete;

            Adapter(Adapter&& sOther) noexcept :
                sEntry(sOther.sEntry),
                sInstance(std::exchange(sOther.sInstance, nullptr)),
                sPhysicalDevice(std::exchange(sOther.sPhysicalDevice, nullptr)),
                sSurface(std::exchange(sOther.sSurface, nullptr)),
                sQueueFamily(std::exchange(sOther.sQueueFamily, std::uint32_t{0}))
            {}

            Adapter& operator=(Adapter&& sOther) noexcept{
                if (this != &sOther){
                    // sEntry tidak di-assign: objek Entry tetap yang dipakai adapter ini.
                    sInstance = std::exchange(sOther.sInstance, nullptr);
                    sPhysicalDevice = std::exchange(sOther.sPhysicalDevice, nullptr);
                    sSurface = std::exchange(sOther.sSurface, nullptr);
                    sQueueFamily = std::exchange(sOther.sQueueFamily, std::uint32_t{0});
                }
                return *this;
            }

            std::expected<Device, Error> createDevice(const Adapter& sAdapter) const;
    };
}

// Instance::requestAdapter() didefinisikan di sini karena butuh tipe lengkap Surface.
std::expected<renaru::Adapter, renaru::Error>
renaru::Instance::requestAdapter(const renaru::Surface& sSurface){
    PFN_vkEnumeratePhysicalDevices pfnEnumeratePhysicalDevices =
        reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
            sEntry.resolveInstance(sInstance, "vkEnumeratePhysicalDevices"));
    if (pfnEnumeratePhysicalDevices == nullptr){
        return std::unexpected(renaru::Error::eAdapterNotFound);
    }

    std::uint32_t sDeviceCount = 0;
    if (pfnEnumeratePhysicalDevices(sInstance, &sDeviceCount, nullptr) != VK_SUCCESS ||
        sDeviceCount == 0){
        return std::unexpected(renaru::Error::eAdapterNotFound);
    }

    std::vector<VkPhysicalDevice> sDevices(sDeviceCount);
    if (pfnEnumeratePhysicalDevices(sInstance, &sDeviceCount, sDevices.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::eAdapterNotFound);
    }

    PFN_vkGetPhysicalDeviceQueueFamilyProperties pfnGetQueueFamilyProperties =
        reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
            sEntry.resolveInstance(sInstance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfnGetSurfaceSupportKHR =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
            sEntry.resolveInstance(sInstance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    if (pfnGetQueueFamilyProperties == nullptr || pfnGetSurfaceSupportKHR == nullptr){
        return std::unexpected(renaru::Error::eAdapterNotFound);
    }

    for (const VkPhysicalDevice sDevice : sDevices){
        std::uint32_t sFamilyCount = 0;
        pfnGetQueueFamilyProperties(sDevice, &sFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> sFamilies(sFamilyCount);
        pfnGetQueueFamilyProperties(sDevice, &sFamilyCount, sFamilies.data());

        for (std::uint32_t sIndex = 0; sIndex < sFamilies.size(); ++sIndex){
            if ((sFamilies[sIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0){
                continue;
            }

            VkBool32 sSupportsPresent = VK_FALSE;
            pfnGetSurfaceSupportKHR(sDevice, sIndex, sSurface.sSurface, &sSupportsPresent);
            if (sSupportsPresent == VK_TRUE){
                std::println("Aru RHI: Adapter(found!!) family={}", sIndex);
                return Adapter(sEntry, sInstance, sDevice, sSurface.sSurface, sIndex);
            }
        }
    }

    return std::unexpected(renaru::Error::eAdapterNotFound);
}
