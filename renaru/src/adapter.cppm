module;

#include <renaru/vulkan.h>

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
        public:
            using ResultType = std::expected<Adapter, Error>;

            // Didefinisikan di :device (butuh tipe Device yang lengkap).
            std::expected<Device, Error> createDevice(const Adapter& sadapter) const;

            // Handle fisik/ssurface dipinjam: salinan tidak diizinkan.
            Adapter(const Adapter&) = delete;
            Adapter& operator=(const Adapter&) = delete;

            Adapter(Adapter&& sother) noexcept :
                sinstance(std::exchange(sother.sinstance, nullptr)),
                sphysicalDevice(std::exchange(sother.sphysicalDevice, nullptr)),
                ssurface(std::exchange(sother.ssurface, nullptr)),
                squeueFamily(std::exchange(sother.squeueFamily, std::uint32_t{0}))
            {}

            Adapter& operator=(Adapter&& sother) noexcept{
                if (this != &sother){
                    sinstance = std::exchange(sother.sinstance, nullptr);
                    sphysicalDevice = std::exchange(sother.sphysicalDevice, nullptr);
                    ssurface = std::exchange(sother.ssurface, nullptr);
                    squeueFamily = std::exchange(sother.squeueFamily, std::uint32_t{0});
                }
                return *this;
            }

            ~Adapter() = default;

            VkPhysicalDevice handle() const noexcept { return sphysicalDevice; }

            VkInstance instanceHandle() const noexcept { return sinstance; }

            VkSurfaceKHR surfaceHandle() const noexcept { return ssurface; }

            std::uint32_t queueFamilyIndex() const noexcept { return squeueFamily; }

            bool empty() const noexcept { return sphysicalDevice == nullptr; }

        private:
            friend class Instance;

            Adapter(VkInstance sinstance, VkPhysicalDevice sphysicalDevice,
                    VkSurfaceKHR ssurface, std::uint32_t squeueFamilyIndex) noexcept :
                sinstance(sinstance),
                sphysicalDevice(sphysicalDevice),
                ssurface(ssurface),
                squeueFamily(squeueFamilyIndex)
            {}

            VkInstance sinstance = nullptr;
            VkPhysicalDevice sphysicalDevice = nullptr;
            VkSurfaceKHR ssurface = nullptr;
            std::uint32_t squeueFamily = 0;
    };
}

// Definisi lintas partisi: Instance didefinisikan di :instance, tetapi
// pemilihan sadapter membutuhkan tipe lengkap Surface dari partisi ini.
std::expected<renaru::Adapter, renaru::Error> renaru::Instance::requestAdapter(const renaru::Surface& ssurface){
    PFN_vkEnumeratePhysicalDevices pfnEnumeratePhysicalDevices =
        reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
            entry().resolveInstance(sinstance, "vkEnumeratePhysicalDevices"));
    if (pfnEnumeratePhysicalDevices == nullptr){
        return std::unexpected(renaru::Error::AdapterNotFound);
    }

    std::uint32_t sdeviceCount = 0;
    if (pfnEnumeratePhysicalDevices(sinstance, &sdeviceCount, nullptr) != VK_SUCCESS ||
        sdeviceCount == 0){
        return std::unexpected(renaru::Error::AdapterNotFound);
    }

    std::vector<VkPhysicalDevice> sdevices(sdeviceCount);
    if (pfnEnumeratePhysicalDevices(sinstance, &sdeviceCount, sdevices.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::AdapterNotFound);
    }

    PFN_vkGetPhysicalDeviceQueueFamilyProperties pfnGetQueueFamilyProperties =
        reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
            entry().resolveInstance(sinstance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfnGetSurfaceSupportKHR =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
            entry().resolveInstance(sinstance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    if (pfnGetQueueFamilyProperties == nullptr || pfnGetSurfaceSupportKHR == nullptr){
        return std::unexpected(renaru::Error::AdapterNotFound);
    }

    for (const VkPhysicalDevice sdevice : sdevices){
        std::uint32_t sfamilyCount = 0;
        pfnGetQueueFamilyProperties(sdevice, &sfamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> sfamilies(sfamilyCount);
        pfnGetQueueFamilyProperties(sdevice, &sfamilyCount, sfamilies.data());

        for (std::uint32_t sindex = 0; sindex < sfamilies.size(); ++sindex){
            if ((sfamilies[sindex].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0){
                continue;
            }

            VkBool32 ssupportsPresent = VK_FALSE;
            pfnGetSurfaceSupportKHR(sdevice, sindex, ssurface.handle(), &ssupportsPresent);
            if (ssupportsPresent == VK_TRUE){
                std::println("Aru RHI: Adapter(found!!) family={}", sindex);
                return Adapter(sinstance, sdevice, ssurface.handle(), sindex);
            }
        }
    }

    return std::unexpected(renaru::Error::AdapterNotFound);
}
