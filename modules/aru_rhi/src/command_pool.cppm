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
    class CommandPool;
}

export namespace aru::rhi {
    // CommandPool memiliki vk::CommandPool untuk satu queue family.
    // Flag eResetCommandBuffer memungkinkan reset per command buffer.
    class CommandPool {
        public:
            using ResultType = std::expected<CommandPool, Error>;

            CommandPool(const CommandPool& other);
            CommandPool(CommandPool&& other) noexcept;
            CommandPool& operator=(const CommandPool& other);
            CommandPool& operator=(CommandPool&& other) noexcept;
            ~CommandPool();

            CommandPool copy() const;
            CommandPool move() noexcept;

            vk::CommandPool handle() const noexcept;
            vk::Device deviceHandle() const noexcept;
            std::uint32_t queueFamilyIndex() const noexcept;
            bool empty() const noexcept;

            // Reset seluruh command buffer yang berasal dari pool ini.
            bool reset(vk::CommandPoolResetFlags flags = {}) noexcept;

        private:
            friend class Device;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            CommandPool(vk::Device device, vk::CommandPool pool,
                        std::uint32_t queueFamilyIndex, InitTag tag) noexcept;
            CommandPool(const CommandPool& other, CopyTag tag) noexcept;
            CommandPool(CommandPool&& other, MoveTag tag) noexcept;

            void release() noexcept;

            vk::Device mDevice;
            vk::CommandPool mPool;
            std::uint32_t mQueueFamily;
    };
}

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

    return CommandPool(this->mDevice, created, this->mQueueFamily, CommandPool::InitTag{});
}

aru::rhi::CommandPool::CommandPool(vk::Device device, vk::CommandPool pool,
                                   std::uint32_t queueFamilyIndex, InitTag) noexcept :
    mDevice(device),
    mPool(pool),
    mQueueFamily(queueFamilyIndex)
{}

aru::rhi::CommandPool::CommandPool(const CommandPool& other) :
    CommandPool(other, CopyTag{})
{}

aru::rhi::CommandPool::CommandPool(CommandPool&& other) noexcept :
    CommandPool(std::move(other), MoveTag{})
{}

aru::rhi::CommandPool::CommandPool(const CommandPool& other, CopyTag) noexcept :
    mDevice(other.mDevice),
    mPool(),
    mQueueFamily(other.mQueueFamily)
{}

aru::rhi::CommandPool::CommandPool(CommandPool&& other, MoveTag) noexcept :
    mDevice(std::exchange(other.mDevice, vk::Device{})),
    mPool(std::exchange(other.mPool, vk::CommandPool{})),
    mQueueFamily(std::exchange(other.mQueueFamily, std::uint32_t{0}))
{}

aru::rhi::CommandPool& aru::rhi::CommandPool::operator=(const CommandPool& other){
    if (this != &other){
        CommandPool fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::CommandPool& aru::rhi::CommandPool::operator=(CommandPool&& other) noexcept{
    if (this != &other){
        this->release();
        this->mDevice = std::exchange(other.mDevice, vk::Device{});
        this->mPool = std::exchange(other.mPool, vk::CommandPool{});
        this->mQueueFamily = std::exchange(other.mQueueFamily, std::uint32_t{0});
    }
    return *this;
}

aru::rhi::CommandPool aru::rhi::CommandPool::copy() const{
    return CommandPool(*this, CopyTag{});
}

aru::rhi::CommandPool aru::rhi::CommandPool::move() noexcept{
    return CommandPool(std::move(*this), MoveTag{});
}

vk::CommandPool aru::rhi::CommandPool::handle() const noexcept{
    return this->mPool;
}

vk::Device aru::rhi::CommandPool::deviceHandle() const noexcept{
    return this->mDevice;
}

std::uint32_t aru::rhi::CommandPool::queueFamilyIndex() const noexcept{
    return this->mQueueFamily;
}

bool aru::rhi::CommandPool::empty() const noexcept{
    return static_cast<VkCommandPool>(this->mPool) == nullptr;
}

bool aru::rhi::CommandPool::reset(vk::CommandPoolResetFlags flags) noexcept{
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

void aru::rhi::CommandPool::release() noexcept{
    if (static_cast<VkCommandPool>(this->mPool) != nullptr &&
        static_cast<VkDevice>(this->mDevice) != nullptr){
        this->mDevice.destroyCommandPool(this->mPool, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        this->mPool = vk::CommandPool{};
    }
    this->mDevice = vk::Device{};
    this->mQueueFamily = 0;
}

aru::rhi::CommandPool::~CommandPool(){
    this->release();
}
