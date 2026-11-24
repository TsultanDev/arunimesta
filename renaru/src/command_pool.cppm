module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <print>
#include <utility>

export module renaru:command_pool;

import :entry;
import :device;
import :queue;

export namespace renaru {
    class CommandPool {
        private:
            using ResultType = std::expected<CommandPool, Error>;

            VkDevice sDevice = nullptr;
            VkCommandPool sPool = nullptr;
            std::uint32_t sQueueFamily = 0;

            CommandPool(VkDevice sDevice, VkCommandPool sPool,
                        std::uint32_t sQueueFamilyIndex) noexcept :
                sDevice(sDevice),
                sPool(sPool),
                sQueueFamily(sQueueFamilyIndex)
            {}

            void sDestroy() noexcept{
                if (sPool != nullptr && sDevice != nullptr){
                    PFN_vkDestroyCommandPool pfnDestroyCommandPool =
                        reinterpret_cast<PFN_vkDestroyCommandPool>(
                            entry().resolveDevice(sDevice, "vkDestroyCommandPool"));
                    if (pfnDestroyCommandPool != nullptr){
                        pfnDestroyCommandPool(sDevice, sPool, nullptr);
                    }
                    sPool = nullptr;
                }
                sDevice = nullptr;
                sQueueFamily = 0;
            }

            friend class Device;

        public:
            ~CommandPool(){ sDestroy(); }

            CommandPool(const CommandPool&) = delete;
            CommandPool& operator=(const CommandPool&) = delete;

            CommandPool(CommandPool&& sOther) noexcept :
                sDevice(std::exchange(sOther.sDevice, nullptr)),
                sPool(std::exchange(sOther.sPool, nullptr)),
                sQueueFamily(std::exchange(sOther.sQueueFamily, std::uint32_t{0}))
            {}

            CommandPool& operator=(CommandPool&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sDevice = std::exchange(sOther.sDevice, nullptr);
                    sPool = std::exchange(sOther.sPool, nullptr);
                    sQueueFamily = std::exchange(sOther.sQueueFamily, std::uint32_t{0});
                }
                return *this;
            }

            bool reset(VkCommandPoolResetFlags sFlags = 0) noexcept{
                if (sPool == nullptr || sDevice == nullptr){
                    return false;
                }
                PFN_vkResetCommandPool pfnResetCommandPool =
                    reinterpret_cast<PFN_vkResetCommandPool>(
                        entry().resolveDevice(sDevice, "vkResetCommandPool"));
                if (pfnResetCommandPool == nullptr){
                    return false;
                }
                return pfnResetCommandPool(sDevice, sPool, sFlags) == VK_SUCCESS;
            }
    };
}

std::expected<renaru::CommandPool, renaru::Error>
renaru::Device::createCommandPool(const renaru::Queue& sQueue) const{
    if (sDevice == nullptr || sQueue.sQueue == nullptr){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }
    if (sQueue.sDevice != sDevice || sQueue.sFamily != sQueueFamily){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }

    VkCommandPoolCreateInfo sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    sInfo.queueFamilyIndex = sQueueFamily;
    sInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    PFN_vkCreateCommandPool pfnCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(
        entry().resolveDevice(sDevice, "vkCreateCommandPool"));
    if (pfnCreateCommandPool == nullptr){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }

    VkCommandPool sCreated = nullptr;
    if (pfnCreateCommandPool(sDevice, &sInfo, nullptr, &sCreated) != VK_SUCCESS ||
        sCreated == nullptr){
        return std::unexpected(renaru::Error::CommandPoolCreationFailed);
    }

    std::println("Aru RHI: CommandPool(created!!) family={}", sQueueFamily);

    return CommandPool(sDevice, sCreated, sQueueFamily);
}
