module;

#include <aru_rhi/vulkan.hpp>

#include <expected>
#include <print>

#include <cstdint>
#include <utility>

export module aru.rhi:command_pool;

import :device;
import :queue;

export namespace aru::rhi {
    // CommandPool memiliki vk::CommandPool untuk satu queue family.
    // Flag eResetCommandBuffer memungkinkan reset per command buffer.
    class CommandPool {
        public:
            using ResultType = std::expected<CommandPool, Error>;

            // vk::CommandPool adalah raw handle: salinan tidak diizinkan.
            CommandPool(const CommandPool&) = delete;
            CommandPool& operator=(const CommandPool&) = delete;

            CommandPool(CommandPool&& other) noexcept :
                mDevice(std::exchange(other.mDevice, vk::Device{})),
                mPool(std::exchange(other.mPool, vk::CommandPool{})),
                mQueueFamily(std::exchange(other.mQueueFamily, std::uint32_t{0}))
            {}

            CommandPool& operator=(CommandPool&& other) noexcept{
                if (this != &other){
                    this->release();
                    this->mDevice = std::exchange(other.mDevice, vk::Device{});
                    this->mPool = std::exchange(other.mPool, vk::CommandPool{});
                    this->mQueueFamily = std::exchange(other.mQueueFamily, std::uint32_t{0});
                }
                return *this;
            }

            ~CommandPool(){ this->release(); }

            vk::CommandPool handle() const noexcept { return this->mPool; }

            vk::Device deviceHandle() const noexcept { return this->mDevice; }

            std::uint32_t queueFamilyIndex() const noexcept { return this->mQueueFamily; }

            bool empty() const noexcept { return static_cast<VkCommandPool>(this->mPool) == nullptr; }

            // Reset seluruh command buffer yang berasal dari pool ini.
            bool reset(vk::CommandPoolResetFlags flags = {}) noexcept{
                if (this->empty()){
                    return false;
                }
                try{
                    this->mDevice.resetCommandPool(this->mPool, flags, VULKAN_HPP_DEFAULT_DISPATCHER);
                }catch(...){
                    return false;
                }
                return true;
            }

        private:
            friend class Device;

            CommandPool(vk::Device device, vk::CommandPool pool,
                        std::uint32_t queueFamilyIndex) noexcept :
                mDevice(device),
                mPool(pool),
                mQueueFamily(queueFamilyIndex)
            {}

            void release() noexcept{
                if (static_cast<VkCommandPool>(this->mPool) != nullptr &&
                    static_cast<VkDevice>(this->mDevice) != nullptr){
                    this->mDevice.destroyCommandPool(this->mPool, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
                    this->mPool = vk::CommandPool{};
                }
                this->mDevice = vk::Device{};
                this->mQueueFamily = 0;
            }

            vk::Device mDevice;
            vk::CommandPool mPool;
            std::uint32_t mQueueFamily;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// CommandPool membutuhkan tipe lengkap Queue/CommandPool dari partisi ini.
std::expected<aru::rhi::CommandPool, aru::rhi::Error>
aru::rhi::Device::createCommandPool(const aru::rhi::Queue& queue) const{
    if (this->empty() || queue.empty()){
        return std::unexpected(aru::rhi::Error::CommandPoolCreationFailed);
    }
    if (static_cast<VkDevice>(queue.deviceHandle()) != static_cast<VkDevice>(this->mDevice)){
        return std::unexpected(aru::rhi::Error::CommandPoolCreationFailed);
    }
    if (queue.familyIndex() != this->mQueueFamily){
        return std::unexpected(aru::rhi::Error::CommandPoolCreationFailed);
    }

    vk::CommandPoolCreateInfo info;
    info.setQueueFamilyIndex(this->mQueueFamily);
    info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    vk::CommandPool created;
    try{
        created = this->mDevice.createCommandPool(info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(aru::rhi::Error::CommandPoolCreationFailed);
    }

    std::println("Aru RHI: CommandPool(created!!) family={}", this->mQueueFamily);

    return CommandPool(this->mDevice, created, this->mQueueFamily);
}
