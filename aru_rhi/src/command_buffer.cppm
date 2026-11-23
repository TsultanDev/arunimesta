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
import :queue;

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

            // Prototipe: command_buffer.record([]{ ... }) otomatis reset +
            // vkBeginCommandBuffer, menjalankan closure, lalu vkEndCommandBuffer.
            template<typename Func>
            std::expected<void, Error> record(Func&& func){
                if (auto started = this->beginRecording(); !started){
                    return std::unexpected(started.error());
                }

                try{
                    std::forward<Func>(func)();
                }catch(...){
                    this->endRecording();
                    const aru::rhi::Error error = aru::rhi::Error::CommandBufferRecordingFailed;
                    if (mOwner != nullptr){
                        mOwner->noteError(error);
                    }
                    return std::unexpected(error);
                }

                this->endRecording();
                if (mOwner != nullptr && mOwner->lastError() != Error::None){
                    return std::unexpected(mOwner->lastError());
                }
                return {};
            }

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

            CommandBuffer(const Device* owner, vk::Device device, vk::CommandPool pool,
                          vk::CommandBuffer buffer, InitTag tag) noexcept;
            CommandBuffer(const CommandBuffer& other, CopyTag tag) noexcept;
            CommandBuffer(CommandBuffer&& other, MoveTag tag) noexcept;

            std::expected<void, Error> beginRecording();
            void endRecording() noexcept;

            void release() noexcept;

            const Device* mOwner;
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
        result.push_back(CommandBuffer(this, this->mDevice, commandPool.handle(),
                                       buffer, CommandBuffer::InitTag{}));
    }

    std::println("Aru RHI: CommandBuffers(allocated!!) count={}", result.size());

    return result;
}

aru::rhi::CommandBuffer::CommandBuffer(const Device* owner, vk::Device device,
                                       vk::CommandPool pool, vk::CommandBuffer buffer,
                                       InitTag) noexcept :
    mOwner(owner),
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
    mOwner(other.mOwner),
    mDevice(other.mDevice),
    mPool(other.mPool),
    mBuffer()
{}

aru::rhi::CommandBuffer::CommandBuffer(CommandBuffer&& other, MoveTag) noexcept :
    mOwner(std::exchange(other.mOwner, nullptr)),
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
        this->mOwner = std::exchange(other.mOwner, nullptr);
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

std::expected<void, aru::rhi::Error> aru::rhi::CommandBuffer::beginRecording(){
    if (this->empty() || mOwner == nullptr || mOwner->empty()){
        return std::unexpected(aru::rhi::Error::CommandBufferRecordingFailed);
    }

    // Pool dibuat dengan eResetCommandBuffer, jadi reset diperbolehkan.
    // Di state awal reset boleh gagal; begin() tetap bisa dipanggil.
    (void)this->reset();
    if (!this->begin()){
        return std::unexpected(aru::rhi::Error::CommandBufferRecordingFailed);
    }

    mOwner->setActiveCommandBuffer(this->mBuffer);
    return {};
}

void aru::rhi::CommandBuffer::endRecording() noexcept{
    if (mOwner != nullptr){
        mOwner->clearActiveCommandBuffer();
    }
    this->end();
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
    this->mOwner = nullptr;
    this->mDevice = vk::Device{};
    this->mPool = vk::CommandPool{};
}

aru::rhi::CommandBuffer::~CommandBuffer(){
    this->release();
}

std::expected<void, aru::rhi::Error> aru::rhi::Queue::submit(
    const aru::rhi::CommandBuffer& buffer) const{
    if (this->empty() || mOwner == nullptr || mOwner->empty() || buffer.empty()){
        return std::unexpected(aru::rhi::Error::CommandSubmissionFailed);
    }
    if (static_cast<VkDevice>(buffer.deviceHandle()) != static_cast<VkDevice>(mOwner->handle())){
        return std::unexpected(aru::rhi::Error::CommandSubmissionFailed);
    }

    const vk::CommandBuffer commandBuffer = buffer.handle();
    const vk::Semaphore waitSemaphore = mOwner->imageAvailableSemaphore();
    const vk::Semaphore signalSemaphore = mOwner->renderFinishedSemaphore();
    const vk::Fence fence = mOwner->currentFence();
    if (static_cast<VkSemaphore>(waitSemaphore) == nullptr ||
        static_cast<VkSemaphore>(signalSemaphore) == nullptr ||
        static_cast<VkFence>(fence) == nullptr){
        return std::unexpected(aru::rhi::Error::CommandSubmissionFailed);
    }

    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };

    vk::SubmitInfo info;
    info.setWaitSemaphoreCount(1);
    info.setPWaitSemaphores(&waitSemaphore);
    info.setPWaitDstStageMask(waitStages);
    info.setCommandBufferCount(1);
    info.setPCommandBuffers(&commandBuffer);
    info.setSignalSemaphoreCount(1);
    info.setPSignalSemaphores(&signalSemaphore);

    try{
        this->mQueue.submit(info, fence, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        mOwner->noteError(aru::rhi::Error::CommandSubmissionFailed);
        return std::unexpected(aru::rhi::Error::CommandSubmissionFailed);
    }
    return {};
}

std::expected<void, aru::rhi::Error> aru::rhi::Queue::present() const{
    if (this->empty() || mOwner == nullptr || mOwner->empty()){
        return std::unexpected(aru::rhi::Error::FramePresentationFailed);
    }

    const vk::SwapchainKHR swapchain = mOwner->swapchainHandle();
    const std::uint32_t imageIndex = mOwner->currentImageIndex();
    const vk::Semaphore waitSemaphore = mOwner->renderFinishedSemaphore();
    if (static_cast<VkSwapchainKHR>(swapchain) == nullptr ||
        static_cast<VkSemaphore>(waitSemaphore) == nullptr){
        return std::unexpected(aru::rhi::Error::FramePresentationFailed);
    }

    vk::PresentInfoKHR info;
    info.setWaitSemaphoreCount(1);
    info.setPWaitSemaphores(&waitSemaphore);
    info.setSwapchainCount(1);
    info.setPSwapchains(&swapchain);
    info.setPImageIndices(&imageIndex);

    try{
        const vk::Result result = this->mQueue.presentKHR(info, VULKAN_HPP_DEFAULT_DISPATCHER);
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR){
            mOwner->noteError(aru::rhi::Error::FramePresentationFailed);
            return std::unexpected(aru::rhi::Error::FramePresentationFailed);
        }
    }catch(...){
        mOwner->noteError(aru::rhi::Error::FramePresentationFailed);
        return std::unexpected(aru::rhi::Error::FramePresentationFailed);
    }
    return {};
}
