module;

#include <renaru/vulkan.h>

#include <cstdint>
#include <expected>
#include <print>
#include <utility>

export module renaru:command_pool;

import :entry;
import :device;
import :queue;

export namespace renaru {
    // CommandPool memiliki VkCommandPool untuk satu queue family.
    // Flag VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT memungkinkan
    // reset per command buffer.
    class CommandPool {
        public:
            using ResultType = std::expected<CommandPool, Error>;

            // VkCommandPool adalah raw handle: salinan tidak diizinkan.
            CommandPool(const CommandPool&) = delete;
            CommandPool& operator=(const CommandPool&) = delete;

            CommandPool(CommandPool&& sother) noexcept :
                sdevice(std::exchange(sother.sdevice, nullptr)),
                spool(std::exchange(sother.spool, nullptr)),
                squeueFamily(std::exchange(sother.squeueFamily, std::uint32_t{0}))
            {}

            CommandPool& operator=(CommandPool&& sother) noexcept{
                if (this != &sother){
                    release();
                    sdevice = std::exchange(sother.sdevice, nullptr);
                    spool = std::exchange(sother.spool, nullptr);
                    squeueFamily = std::exchange(sother.squeueFamily, std::uint32_t{0});
                }
                return *this;
            }

            ~CommandPool(){ release(); }

            VkCommandPool handle() const noexcept { return spool; }

            VkDevice deviceHandle() const noexcept { return sdevice; }

            std::uint32_t queueFamilyIndex() const noexcept { return squeueFamily; }

            bool empty() const noexcept { return spool == nullptr; }

            // Reset seluruh command buffer yang berasal dari pool ini.
            bool reset(VkCommandPoolResetFlags sflags = 0) noexcept{
                if (spool == nullptr || sdevice == nullptr){
                    return false;
                }
                PFN_vkResetCommandPool pfnResetCommandPool =
                    reinterpret_cast<PFN_vkResetCommandPool>(
                        entry().resolveDevice(sdevice, "vkResetCommandPool"));
                if (pfnResetCommandPool == nullptr){
                    return false;
                }
                return pfnResetCommandPool(sdevice, spool, sflags) == VK_SUCCESS;
            }

        private:
            friend class Device;

            CommandPool(VkDevice sdevice, VkCommandPool spool,
                        std::uint32_t squeueFamilyIndex) noexcept :
                sdevice(sdevice),
                spool(spool),
                squeueFamily(squeueFamilyIndex)
            {}

            void release() noexcept{
                if (spool != nullptr && sdevice != nullptr){
                    PFN_vkDestroyCommandPool pfnDestroyCommandPool =
                        reinterpret_cast<PFN_vkDestroyCommandPool>(
                            entry().resolveDevice(sdevice, "vkDestroyCommandPool"));
                    if (pfnDestroyCommandPool != nullptr){
                        pfnDestroyCommandPool(sdevice, spool, nullptr);
                    }
                    spool = nullptr;
                }
                sdevice = nullptr;
                squeueFamily = 0;
            }

            VkDevice sdevice = nullptr;
            VkCommandPool spool = nullptr;
            std::uint32_t squeueFamily = 0;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// CommandPool membutuhkan tipe lengkap Queue/CommandPool dari partisi ini.
std::expected<renaru::CommandPool, renaru::Error>
renaru::Device::createCommandPool(const renaru::Queue& squeue) const{
    if (sdevice == nullptr || squeue.empty()){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }
    if (squeue.deviceHandle() != sdevice){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }
    if (squeue.familyIndex() != squeueFamily){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }

    VkCommandPoolCreateInfo sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    sinfo.queueFamilyIndex = squeueFamily;
    sinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    PFN_vkCreateCommandPool pfnCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(
        entry().resolveDevice(sdevice, "vkCreateCommandPool"));
    if (pfnCreateCommandPool == nullptr){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }

    VkCommandPool screated = nullptr;
    if (pfnCreateCommandPool(sdevice, &sinfo, nullptr, &screated) != VK_SUCCESS ||
        screated == nullptr){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }

    std::println("Aru RHI: CommandPool(created!!) family={}", squeueFamily);

    return CommandPool(sdevice, screated, squeueFamily);
}
