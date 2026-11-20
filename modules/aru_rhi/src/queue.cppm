module;

#include <aru_rhi/vulkan.hpp>

#include <cstdint>
#include <utility>

export module aru.rhi:queue;

import aru.standard;
import :instance;
import :device;

export namespace aru::rhi {
    class Queue;
}

export namespace aru::rhi {
    class Queue {
        public:
            using ResultType = aru::standard::Result<Queue, Error>;

            Queue(const Queue& other);
            Queue(Queue&& other) noexcept;
            Queue& operator=(const Queue& other);
            Queue& operator=(Queue&& other) noexcept;
            ~Queue() = default;

            Queue copy() const;
            Queue move() noexcept;

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

            Queue(vk::Device device, vk::Queue queue, QueueFlags flags,
                  std::uint32_t familyIndex, InitTag tag) noexcept;
            Queue(const Queue& other, CopyTag tag);
            Queue(Queue&& other, MoveTag tag) noexcept;

            vk::Device mDevice;
            vk::Queue mQueue;
            QueueFlags mFlags;
            std::uint32_t mFamily;
    };
}

aru::standard::Result<aru::rhi::Queue, aru::rhi::Error> aru::rhi::Device::requestQueue(aru::rhi::QueueFlags flags) const{
    if (this->empty()){
        return aru::rhi::Error::QueueRequestFailed;
    }

    const auto families = this->mPhysicalDevice.getQueueFamilyProperties(VULKAN_HPP_DEFAULT_DISPATCHER);
    if (this->mQueueFamily >= families.size()){
        return aru::rhi::Error::QueueRequestFailed;
    }
    const VkQueueFlags available = static_cast<VkQueueFlags>(families[this->mQueueFamily].queueFlags);

    if (hasFlag(flags, QueueFlags::Graphics) && (available & VK_QUEUE_GRAPHICS_BIT) == 0){
        return aru::rhi::Error::QueueRequestFailed;
    }
    if (hasFlag(flags, QueueFlags::Compute) && (available & VK_QUEUE_COMPUTE_BIT) == 0){
        return aru::rhi::Error::QueueRequestFailed;
    }
    if (hasFlag(flags, QueueFlags::Transfer) && (available & VK_QUEUE_TRANSFER_BIT) == 0){
        return aru::rhi::Error::QueueRequestFailed;
    }
    if (hasFlag(flags, QueueFlags::Present)){
        const vk::Bool32 supportsPresent = this->mPhysicalDevice.getSurfaceSupportKHR(
            this->mQueueFamily, this->mSurface, VULKAN_HPP_DEFAULT_DISPATCHER
        );
        if (!supportsPresent){
            return aru::rhi::Error::QueueRequestFailed;
        }
    }

    const vk::Queue queue = this->mDevice.getQueue(this->mQueueFamily, 0, VULKAN_HPP_DEFAULT_DISPATCHER);
    if (static_cast<VkQueue>(queue) == nullptr){
        return aru::rhi::Error::QueueRequestFailed;
    }

    aru::standard::println("Aru RHI: Queue(requested!!) family={}", this->mQueueFamily);

    return Queue(this->mDevice, queue, flags, this->mQueueFamily, Queue::InitTag{});
}

aru::rhi::Queue::Queue(vk::Device device, vk::Queue queue, QueueFlags flags,
                       std::uint32_t familyIndex, InitTag) noexcept :
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
    mDevice(other.mDevice),
    mQueue(other.mQueue),
    mFlags(other.mFlags),
    mFamily(other.mFamily)
{}

aru::rhi::Queue::Queue(Queue&& other, MoveTag) noexcept :
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
