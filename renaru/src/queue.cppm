module;

#include <renaru/vulkan.h>

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
    // Forward declaration: tipe lengkap ada di partisi :command_buffer.
    class CommandBuffer;
}

export namespace renaru {
    class Queue {
        public:
            using ResultType = std::expected<Queue, Error>;

            // Didefinisikan di :command_buffer karena butuh CommandBuffer lengkap.
            std::expected<void, Error> submit(const CommandBuffer& sbuffer) const;
            std::expected<void, Error> present() const;

            // VkQueue tidak dimiliki aplikasi, tetapi handle-nya tetap raw:
            // salinan tidak diizinkan.
            Queue(const Queue&) = delete;
            Queue& operator=(const Queue&) = delete;

            Queue(Queue&& sother) noexcept :
                sowner(std::exchange(sother.sowner, nullptr)),
                sdevice(std::exchange(sother.sdevice, nullptr)),
                squeue(std::exchange(sother.squeue, nullptr)),
                sflags(std::exchange(sother.sflags, QueueFlags::None)),
                sfamily(std::exchange(sother.sfamily, std::uint32_t{0}))
            {}

            Queue& operator=(Queue&& sother) noexcept{
                if (this != &sother){
                    sowner = std::exchange(sother.sowner, nullptr);
                    sdevice = std::exchange(sother.sdevice, nullptr);
                    squeue = std::exchange(sother.squeue, nullptr);
                    sflags = std::exchange(sother.sflags, QueueFlags::None);
                    sfamily = std::exchange(sother.sfamily, std::uint32_t{0});
                }
                return *this;
            }

            ~Queue() = default;

            VkQueue handle() const noexcept { return squeue; }

            VkDevice deviceHandle() const noexcept { return sdevice; }

            QueueFlags flags() const noexcept { return sflags; }

            std::uint32_t familyIndex() const noexcept { return sfamily; }

            bool empty() const noexcept { return squeue == nullptr; }

        private:
            friend class Device;

            Queue(const Device* sowner, VkDevice sdevice, VkQueue squeue,
                  QueueFlags sflags, std::uint32_t sfamilyIndex) noexcept :
                sowner(sowner),
                sdevice(sdevice),
                squeue(squeue),
                sflags(sflags),
                sfamily(sfamilyIndex)
            {}

            const Device* sowner = nullptr;
            VkDevice sdevice = nullptr;
            VkQueue squeue = nullptr;
            QueueFlags sflags = QueueFlags::None;
            std::uint32_t sfamily = 0;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi
// permintaan queue membutuhkan tipe lengkap Queue dari partisi ini.
std::expected<renaru::Queue, renaru::Error> renaru::Device::requestQueue(renaru::QueueFlags sflags) const{
    if (sdevice == nullptr){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }

    PFN_vkGetPhysicalDeviceQueueFamilyProperties pfnGetQueueFamilyProperties =
        reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
            entry().resolveInstance(sinstance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    if (pfnGetQueueFamilyProperties == nullptr){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }

    std::uint32_t sfamilyCount = 0;
    pfnGetQueueFamilyProperties(sphysicalDevice, &sfamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> sfamilies(sfamilyCount);
    pfnGetQueueFamilyProperties(sphysicalDevice, &sfamilyCount, sfamilies.data());

    if (squeueFamily >= sfamilies.size()){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }
    const VkQueueFlags savailable = sfamilies[squeueFamily].queueFlags;

    if (hasFlag(sflags, QueueFlags::Graphics) && (savailable & VK_QUEUE_GRAPHICS_BIT) == 0){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }
    if (hasFlag(sflags, QueueFlags::Compute) && (savailable & VK_QUEUE_COMPUTE_BIT) == 0){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }
    if (hasFlag(sflags, QueueFlags::Transfer) && (savailable & VK_QUEUE_TRANSFER_BIT) == 0){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }
    if (hasFlag(sflags, QueueFlags::Present)){
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfnGetSurfaceSupportKHR =
            reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
                entry().resolveInstance(sinstance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
        VkBool32 ssupportsPresent = VK_FALSE;
        if (pfnGetSurfaceSupportKHR == nullptr){
            return std::unexpected(renaru::Error::QueueRequestFailed);
        }
        pfnGetSurfaceSupportKHR(sphysicalDevice, squeueFamily, ssurface,
                                &ssupportsPresent);
        if (ssupportsPresent != VK_TRUE){
            return std::unexpected(renaru::Error::QueueRequestFailed);
        }
    }

    PFN_vkGetDeviceQueue pfnGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(
        entry().resolveDevice(sdevice, "vkGetDeviceQueue"));
    if (pfnGetDeviceQueue == nullptr){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }

    VkQueue squeue = nullptr;
    pfnGetDeviceQueue(sdevice, squeueFamily, 0, &squeue);
    if (squeue == nullptr){
        return std::unexpected(renaru::Error::QueueRequestFailed);
    }

    std::println("Aru RHI: Queue(requested!!) family={}", squeueFamily);

    return Queue(this, sdevice, squeue, sflags, squeueFamily);
}
