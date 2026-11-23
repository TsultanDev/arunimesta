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
    // Forward declaration: tipe lengkap ada di partisi :command_buffer.
    class CommandBuffer;
}

export namespace aru::rhi {
    class Queue {
        private:
            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

        public:
            using ResultType = std::expected<Queue, Error>;

            Queue(const Queue& other) : Queue(other, CopyTag{}) {}

            Queue(Queue&& other) noexcept : Queue(std::move(other), MoveTag{}) {}

            Queue& operator=(const Queue& other){
                if (this != &other){
                    Queue fresh(other, CopyTag{});
                    *this = std::move(fresh);
                }
                return *this;
            }

            Queue& operator=(Queue&& other) noexcept{
                if (this != &other){
                    this->mOwner = std::exchange(other.mOwner, nullptr);
                    this->mDevice = std::exchange(other.mDevice, vk::Device{});
                    this->mQueue = std::exchange(other.mQueue, vk::Queue{});
                    this->mFlags = std::exchange(other.mFlags, QueueFlags::None);
                    this->mFamily = std::exchange(other.mFamily, std::uint32_t{0});
                }
                return *this;
            }

            ~Queue() = default;

            Queue copy() const { return Queue(*this, CopyTag{}); }

            Queue move() noexcept { return Queue(std::move(*this), MoveTag{}); }

            // Didefinisikan di :command_buffer karena butuh CommandBuffer lengkap.
            std::expected<void, Error> submit(const CommandBuffer& buffer) const;
            std::expected<void, Error> present() const;

            vk::Queue handle() const noexcept { return this->mQueue; }

            vk::Device deviceHandle() const noexcept { return this->mDevice; }

            QueueFlags flags() const noexcept { return this->mFlags; }

            std::uint32_t familyIndex() const noexcept { return this->mFamily; }

            bool empty() const noexcept { return static_cast<VkQueue>(this->mQueue) == nullptr; }

        private:
            friend class Device;

            Queue(const Device* owner, vk::Device device, vk::Queue queue,
                  QueueFlags flags, std::uint32_t familyIndex, InitTag) noexcept :
                mOwner(owner),
                mDevice(device),
                mQueue(queue),
                mFlags(flags),
                mFamily(familyIndex)
            {}

            Queue(const Queue& other, CopyTag) :
                mOwner(other.mOwner),
                mDevice(other.mDevice),
                mQueue(other.mQueue),
                mFlags(other.mFlags),
                mFamily(other.mFamily)
            {}

            Queue(Queue&& other, MoveTag) noexcept :
                mOwner(std::exchange(other.mOwner, nullptr)),
                mDevice(std::exchange(other.mDevice, vk::Device{})),
                mQueue(std::exchange(other.mQueue, vk::Queue{})),
                mFlags(std::exchange(other.mFlags, QueueFlags::None)),
                mFamily(std::exchange(other.mFamily, std::uint32_t{0}))
            {}

            const Device* mOwner;
            vk::Device mDevice;
            vk::Queue mQueue;
            QueueFlags mFlags;
            std::uint32_t mFamily;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi
// permintaan queue membutuhkan tipe lengkap Queue dari partisi ini.
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
