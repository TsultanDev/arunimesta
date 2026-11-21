module;

#include <aru_rhi/vulkan.hpp>

#include <expected>
#include <print>

#include <cstdint>
#include <utility>
#include <vector>

export module aru.rhi:command_buffer;

import :device;
import :command_pool;

export namespace aru::rhi {
    class CommandBuffer;
}

export namespace aru::rhi {
    // CommandBuffer adalah wrapper untuk satu vk::CommandBuffer primary.
    // Buffer dialokasikan sekaligus dari CommandPool oleh
    // Device::allocateCommandBuffers(); setiap wrapper membebaskan handle-nya
    // sendiri, jadi CommandPool harus tetap hidup lebih lama dari buffer.
    class CommandBuffer {
        public:
            using ResultType = std::expected<CommandBuffer, Error>;

            CommandBuffer(const CommandBuffer& other);
            CommandBuffer(CommandBuffer&& other) noexcept;
            CommandBuffer& operator=(const CommandBuffer& other);
            CommandBuffer& operator=(CommandBuffer&& other) noexcept;
            ~CommandBuffer();

            CommandBuffer copy() const;
            CommandBuffer move() noexcept;

            vk::CommandBuffer handle() const noexcept;
            vk::Device deviceHandle() const noexcept;
            vk::CommandPool poolHandle() const noexcept;
            bool empty() const noexcept;

            // Recording lifecycle. Mengembalikan false bila Vulkan menolak
            // operasi (misalnya buffer belum siap atau sudah direkam).
            bool begin(vk::CommandBufferUsageFlags flags = {}) noexcept;
            void end() noexcept;
            bool reset(vk::CommandBufferResetFlags flags = {}) noexcept;

        private:
            friend class Device;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            CommandBuffer(vk::Device device, vk::CommandPool pool,
                          vk::CommandBuffer buffer, InitTag tag) noexcept;
            CommandBuffer(const CommandBuffer& other, CopyTag tag) noexcept;
            CommandBuffer(CommandBuffer&& other, MoveTag tag) noexcept;

            void release() noexcept;

            vk::Device mDevice;
            vk::CommandPool mPool;
            vk::CommandBuffer mBuffer;
    };
}

std::expected<std::vector<aru::rhi::CommandBuffer>, aru::rhi::Error>
aru::rhi::Device::allocateCommandBuffers(const aru::rhi::CommandPool& commandPool,
                                         std::uint32_t count) const{
    if (this->empty() || commandPool.empty() || count == 0){
        return std::unexpected(aru::rhi::Error::CommandBufferAllocationFailed);
    }
    if (static_cast<VkDevice>(commandPool.deviceHandle()) != static_cast<VkDevice>(this->mDevice)){
        return std::unexpected(aru::rhi::Error::CommandBufferAllocationFailed);
    }

    vk::CommandBufferAllocateInfo info;
    info.setCommandPool(commandPool.handle());
    info.setLevel(vk::CommandBufferLevel::ePrimary);
    info.setCommandBufferCount(count);

    std::vector<vk::CommandBuffer> allocated;
    try{
        allocated = this->mDevice.allocateCommandBuffers(info, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(aru::rhi::Error::CommandBufferAllocationFailed);
    }
    if (allocated.size() != count){
        for (const vk::CommandBuffer& buffer : allocated){
            if (static_cast<VkCommandBuffer>(buffer) != nullptr){
                this->mDevice.freeCommandBuffers(commandPool.handle(), 1, &buffer,
                                                 VULKAN_HPP_DEFAULT_DISPATCHER);
            }
        }
        return std::unexpected(aru::rhi::Error::CommandBufferAllocationFailed);
    }

    std::vector<CommandBuffer> result;
    result.reserve(allocated.size());
    for (const vk::CommandBuffer& buffer : allocated){
        result.push_back(CommandBuffer(this->mDevice, commandPool.handle(),
                                       buffer, CommandBuffer::InitTag{}));
    }

    std::println("Aru RHI: CommandBuffers(allocated!!) count={}", result.size());

    return result;
}

aru::rhi::CommandBuffer::CommandBuffer(vk::Device device, vk::CommandPool pool,
                                       vk::CommandBuffer buffer, InitTag) noexcept :
    mDevice(device),
    mPool(pool),
    mBuffer(buffer)
{}

aru::rhi::CommandBuffer::CommandBuffer(const CommandBuffer& other) :
    CommandBuffer(other, CopyTag{})
{}

aru::rhi::CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept :
    CommandBuffer(std::move(other), MoveTag{})
{}

aru::rhi::CommandBuffer::CommandBuffer(const CommandBuffer& other, CopyTag) noexcept :
    mDevice(other.mDevice),
    mPool(other.mPool),
    mBuffer()
{}

aru::rhi::CommandBuffer::CommandBuffer(CommandBuffer&& other, MoveTag) noexcept :
    mDevice(std::exchange(other.mDevice, vk::Device{})),
    mPool(std::exchange(other.mPool, vk::CommandPool{})),
    mBuffer(std::exchange(other.mBuffer, vk::CommandBuffer{}))
{}

aru::rhi::CommandBuffer& aru::rhi::CommandBuffer::operator=(const CommandBuffer& other){
    if (this != &other){
        CommandBuffer fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::CommandBuffer& aru::rhi::CommandBuffer::operator=(CommandBuffer&& other) noexcept{
    if (this != &other){
        this->release();
        this->mDevice = std::exchange(other.mDevice, vk::Device{});
        this->mPool = std::exchange(other.mPool, vk::CommandPool{});
        this->mBuffer = std::exchange(other.mBuffer, vk::CommandBuffer{});
    }
    return *this;
}

aru::rhi::CommandBuffer aru::rhi::CommandBuffer::copy() const{
    return CommandBuffer(*this, CopyTag{});
}

aru::rhi::CommandBuffer aru::rhi::CommandBuffer::move() noexcept{
    return CommandBuffer(std::move(*this), MoveTag{});
}

vk::CommandBuffer aru::rhi::CommandBuffer::handle() const noexcept{
    return this->mBuffer;
}

vk::Device aru::rhi::CommandBuffer::deviceHandle() const noexcept{
    return this->mDevice;
}

vk::CommandPool aru::rhi::CommandBuffer::poolHandle() const noexcept{
    return this->mPool;
}

bool aru::rhi::CommandBuffer::empty() const noexcept{
    return static_cast<VkCommandBuffer>(this->mBuffer) == nullptr;
}

bool aru::rhi::CommandBuffer::begin(vk::CommandBufferUsageFlags flags) noexcept{
    if (this->empty()){
        return false;
    }
    vk::CommandBufferBeginInfo info;
    info.setFlags(flags);
    try{
        this->mBuffer.begin(info, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return false;
    }
    return true;
}

void aru::rhi::CommandBuffer::end() noexcept{
    if (this->empty()){
        return;
    }
    try{
        this->mBuffer.end(VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        // Status dicatat oleh validation layer; API tidak melempar exception.
    }
}

bool aru::rhi::CommandBuffer::reset(vk::CommandBufferResetFlags flags) noexcept{
    if (this->empty()){
        return false;
    }
    try{
        this->mBuffer.reset(flags, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return false;
    }
    return true;
}

void aru::rhi::CommandBuffer::release() noexcept{
    if (static_cast<VkCommandBuffer>(this->mBuffer) != nullptr &&
        static_cast<VkDevice>(this->mDevice) != nullptr &&
        static_cast<VkCommandPool>(this->mPool) != nullptr){
        this->mDevice.freeCommandBuffers(this->mPool, 1, &this->mBuffer,
                                         VULKAN_HPP_DEFAULT_DISPATCHER);
        this->mBuffer = vk::CommandBuffer{};
    }
    this->mDevice = vk::Device{};
    this->mPool = vk::CommandPool{};
}

aru::rhi::CommandBuffer::~CommandBuffer(){
    this->release();
}
