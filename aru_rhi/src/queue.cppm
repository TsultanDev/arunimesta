module;

#include <aru_rhi/vulkan.hpp>

#include <expected>
#include <print>

#include <cstdint>
#include <utility>

export module aru.rhi:queue;

import :instance;
import :device;

export namespace aru::rhi {
    class CommandBuffer;
    class Queue;
}

export namespace aru::rhi {
    class Queue {
        public:
            using ResultType = std::expected<Queue, Error>;

            Queue(const Queue& other);
            Queue(Queue&& other) noexcept;
            Queue& operator=(const Queue& other);
            Queue& operator=(Queue&& other) noexcept;
            ~Queue() = default;

            Queue copy() const;
            Queue move() noexcept;

            // Didefinisikan di :command_buffer karena butuh CommandBuffer lengkap.
            std::expected<void, Error> submit(const CommandBuffer& buffer) const;
            std::expected<void, Error> present() const;

            vk::Queue handle() const noexcept;
            vk::Device deviceHandle() const noexcept;
            QueueFlags flags() const noexcept;
            std::uint32_t familyIndex() const noexcept;
            bool empty() const noexcept;

        private:
            friend class Device;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Queue(const Device* owner, vk::Device device, vk::Queue queue,
                  QueueFlags flags, std::uint32_t familyIndex, InitTag tag) noexcept;
            Queue(const Queue& other, CopyTag tag);
            Queue(Queue&& other, MoveTag tag) noexcept;

            const Device* mOwner;
            vk::Device mDevice;
            vk::Queue mQueue;
            QueueFlags mFlags;
            std::uint32_t mFamily;
    };
}

std::expected<aru::rhi::Queue, aru::rhi::Error> aru::rhi::Device::requestQueue(aru::rhi::QueueFlags flags) const{
    if (this->empty()){
        return std::unexpected(aru::rhi::Error::QueueRequestFailed);
    }

    const auto families = this->mPhysicalDevice.getQueueFamilyProperties(VULKAN_HPP_DEFAULT_DISPATCHER);
    if (this->mQueueFamily >= families.size()){
        return std::unexpected(aru::rhi::Error::QueueRequestFailed);
    }
    const VkQueueFlags available = static_cast<VkQueueFlags>(families[this->mQueueFamily].queueFlags);

    if (hasFlag(flags, QueueFlags::Graphics) && (available & VK_QUEUE_GRAPHICS_BIT) == 0){
        return std::unexpected(aru::rhi::Error::QueueRequestFailed);
    }
    if (hasFlag(flags, QueueFlags::Compute) && (available & VK_QUEUE_COMPUTE_BIT) == 0){
        return std::unexpected(aru::rhi::Error::QueueRequestFailed);
    }
    if (hasFlag(flags, QueueFlags::Transfer) && (available & VK_QUEUE_TRANSFER_BIT) == 0){
        return std::unexpected(aru::rhi::Error::QueueRequestFailed);
    }
    if (hasFlag(flags, QueueFlags::Present)){
        const vk::Bool32 supportsPresent = this->mPhysicalDevice.getSurfaceSupportKHR(
            this->mQueueFamily, this->mSurface, VULKAN_HPP_DEFAULT_DISPATCHER
        );
        if (!supportsPresent){
            return std::unexpected(aru::rhi::Error::QueueRequestFailed);
        }
    }

    const vk::Queue queue = this->mDevice.getQueue(this->mQueueFamily, 0, VULKAN_HPP_DEFAULT_DISPATCHER);
    if (static_cast<VkQueue>(queue) == nullptr){
        return std::unexpected(aru::rhi::Error::QueueRequestFailed);
    }

    std::println("Aru RHI: Queue(requested!!) family={}", this->mQueueFamily);

    return Queue(this, this->mDevice, queue, flags, this->mQueueFamily, Queue::InitTag{});
}

aru::rhi::Queue::Queue(const Device* owner, vk::Device device, vk::Queue queue,
                       QueueFlags flags, std::uint32_t familyIndex, InitTag) noexcept :
    mOwner(owner),
    mDevice(device),
    mQueue(queue),
    mFlags(flags),
    mFamily(familyIndex)
{}

aru::rhi::Queue::Queue(const Queue& other) :
    Queue(other, CopyTag{})
{}

aru::rhi::Queue::Queue(Queue&& other) noexcept :
    Queue(std::move(other), MoveTag{})
{}

aru::rhi::Queue::Queue(const Queue& other, CopyTag) :
    mOwner(other.mOwner),
    mDevice(other.mDevice),
    mQueue(other.mQueue),
    mFlags(other.mFlags),
    mFamily(other.mFamily)
{}

aru::rhi::Queue::Queue(Queue&& other, MoveTag) noexcept :
    mOwner(std::exchange(other.mOwner, nullptr)),
    mDevice(std::exchange(other.mDevice, vk::Device{})),
    mQueue(std::exchange(other.mQueue, vk::Queue{})),
    mFlags(std::exchange(other.mFlags, QueueFlags::None)),
    mFamily(std::exchange(other.mFamily, std::uint32_t{0}))
{}

aru::rhi::Queue& aru::rhi::Queue::operator=(const Queue& other){
    if (this != &other){
        Queue fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::Queue& aru::rhi::Queue::operator=(Queue&& other) noexcept{
    if (this != &other){
        this->mOwner = std::exchange(other.mOwner, nullptr);
        this->mDevice = std::exchange(other.mDevice, vk::Device{});
        this->mQueue = std::exchange(other.mQueue, vk::Queue{});
        this->mFlags = std::exchange(other.mFlags, QueueFlags::None);
        this->mFamily = std::exchange(other.mFamily, std::uint32_t{0});
    }
    return *this;
}

aru::rhi::Queue aru::rhi::Queue::copy() const{
    return Queue(*this, CopyTag{});
}

aru::rhi::Queue aru::rhi::Queue::move() noexcept{
    return Queue(std::move(*this), MoveTag{});
}

vk::Queue aru::rhi::Queue::handle() const noexcept{
    return this->mQueue;
}

vk::Device aru::rhi::Queue::deviceHandle() const noexcept{
    return this->mDevice;
}

aru::rhi::QueueFlags aru::rhi::Queue::flags() const noexcept{
    return this->mFlags;
}

std::uint32_t aru::rhi::Queue::familyIndex() const noexcept{
    return this->mFamily;
}

bool aru::rhi::Queue::empty() const noexcept{
    return static_cast<VkQueue>(this->mQueue) == nullptr;
}
