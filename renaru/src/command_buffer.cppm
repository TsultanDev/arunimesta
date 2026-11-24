module;

#include <renaru/vulkan.hpp>

#include <expected>
#include <print>

#include <cstdint>
#include <utility>
#include <vector>

export module renaru:command_buffer;

import :device;
import :command_pool;
import :queue;

export namespace renaru {
    class CommandBuffer {
        public:
            using ResultType = std::expected<CommandBuffer, Error>;

            // vk::CommandBuffer adalah raw handle: salinan tidak diizinkan.
            CommandBuffer(const CommandBuffer&) = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;

            CommandBuffer(CommandBuffer&& other) noexcept :
                mOwner(std::exchange(other.mOwner, nullptr)),
                mDevice(std::exchange(other.mDevice, vk::Device{})),
                mPool(std::exchange(other.mPool, vk::CommandPool{})),
                mBuffer(std::exchange(other.mBuffer, vk::CommandBuffer{}))
            {}

            CommandBuffer& operator=(CommandBuffer&& other) noexcept{
                if (this != &other){
                    this->release();
                    this->mOwner = std::exchange(other.mOwner, nullptr);
                    this->mDevice = std::exchange(other.mDevice, vk::Device{});
                    this->mPool = std::exchange(other.mPool, vk::CommandPool{});
                    this->mBuffer = std::exchange(other.mBuffer, vk::CommandBuffer{});
                }
                return *this;
            }

            ~CommandBuffer(){ this->release(); }

            template<typename Func>
            std::expected<void, Error> record(Func&& func){
                if (auto started = this->beginRecording(); !started){
                    return std::unexpected(started.error());
                }

                try{
                    std::forward<Func>(func)();
                }catch(...){
                    this->endRecording();
                    const renaru::Error error = renaru::Error::CommandBufferRecordingFailed;
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

            vk::CommandBuffer handle() const noexcept { return this->mBuffer; }

            vk::Device deviceHandle() const noexcept { return this->mDevice; }

            vk::CommandPool poolHandle() const noexcept { return this->mPool; }

            bool empty() const noexcept { return static_cast<VkCommandBuffer>(this->mBuffer) == nullptr; }

            bool begin(vk::CommandBufferUsageFlags flags = {}) noexcept{
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

            void end() noexcept{
                if (this->empty()){
                    return;
                }
                try{
                    this->mBuffer.end(VULKAN_HPP_DEFAULT_DISPATCHER);
                }catch(...){
                    // Status dicatat oleh validation layer; API tidak melempar exception.
                }
            }

            bool reset(vk::CommandBufferResetFlags flags = {}) noexcept{
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

        private:
            friend class Device;

            CommandBuffer(const Device* owner, vk::Device device, vk::CommandPool pool,
                          vk::CommandBuffer buffer) noexcept :
                mOwner(owner),
                mDevice(device),
                mPool(pool),
                mBuffer(buffer)
            {}

            std::expected<void, Error> beginRecording(){
                if (this->empty() || mOwner == nullptr || mOwner->empty()){
                    return std::unexpected(renaru::Error::CommandBufferRecordingFailed);
                }

                // Pool dibuat dengan eResetCommandBuffer, jadi reset diperbolehkan.
                // Di state awal reset boleh gagal; begin() tetap bisa dipanggil.
                (void)this->reset();
                if (!this->begin()){
                    return std::unexpected(renaru::Error::CommandBufferRecordingFailed);
                }

                mOwner->setActiveCommandBuffer(this->mBuffer);
                return {};
            }

            void endRecording() noexcept{
                if (mOwner != nullptr){
                    mOwner->clearActiveCommandBuffer();
                }
                this->end();
            }

            void release() noexcept{
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

            const Device* mOwner;
            vk::Device mDevice;
            vk::CommandPool mPool;
            vk::CommandBuffer mBuffer;
    };
}

// Definisi lintas partisi: Device/Queue didefinisikan di partisi sebelumnya,
// tetapi alokasi dan submit membutuhkan tipe lengkap CommandBuffer di sini.
std::expected<std::vector<renaru::CommandBuffer>, renaru::Error>
renaru::Device::allocateCommandBuffers(const renaru::CommandPool& commandPool,
                                         std::uint32_t count) const{
    if (this->empty() || commandPool.empty() || count == 0){
        return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
    }
    if (static_cast<VkDevice>(commandPool.deviceHandle()) != static_cast<VkDevice>(this->mDevice)){
        return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
    }

    vk::CommandBufferAllocateInfo info;
    info.setCommandPool(commandPool.handle());
    info.setLevel(vk::CommandBufferLevel::ePrimary);
    info.setCommandBufferCount(count);

    std::vector<vk::CommandBuffer> allocated;
    try{
        allocated = this->mDevice.allocateCommandBuffers(info, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
    }
    if (allocated.size() != count){
        for (const vk::CommandBuffer& buffer : allocated){
            if (static_cast<VkCommandBuffer>(buffer) != nullptr){
                this->mDevice.freeCommandBuffers(commandPool.handle(), 1, &buffer,
                                                 VULKAN_HPP_DEFAULT_DISPATCHER);
            }
        }
        return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
    }

    std::vector<CommandBuffer> result;
    result.reserve(allocated.size());
    for (const vk::CommandBuffer& buffer : allocated){
        result.push_back(CommandBuffer(this, this->mDevice, commandPool.handle(),
                                       buffer));
    }

    std::println("Aru RHI: CommandBuffers(allocated!!) count={}", result.size());

    return result;
}

std::expected<void, renaru::Error> renaru::Queue::submit(
    const renaru::CommandBuffer& buffer) const{
    if (this->empty() || mOwner == nullptr || mOwner->empty() || buffer.empty()){
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
    }
    if (static_cast<VkDevice>(buffer.deviceHandle()) != static_cast<VkDevice>(mOwner->handle())){
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
    }

    const vk::CommandBuffer commandBuffer = buffer.handle();
    const vk::Semaphore waitSemaphore = mOwner->imageAvailableSemaphore();
    const vk::Semaphore signalSemaphore = mOwner->renderFinishedSemaphore();
    const vk::Fence fence = mOwner->currentFence();
    if (static_cast<VkSemaphore>(waitSemaphore) == nullptr ||
        static_cast<VkSemaphore>(signalSemaphore) == nullptr ||
        static_cast<VkFence>(fence) == nullptr){
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
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
        mOwner->noteError(renaru::Error::CommandSubmissionFailed);
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
    }
    return {};
}

std::expected<void, renaru::Error> renaru::Queue::present() const{
    if (this->empty() || mOwner == nullptr || mOwner->empty()){
        return std::unexpected(renaru::Error::FramePresentationFailed);
    }

    const vk::SwapchainKHR swapchain = mOwner->swapchainHandle();
    const std::uint32_t imageIndex = mOwner->currentImageIndex();
    const vk::Semaphore waitSemaphore = mOwner->renderFinishedSemaphore();
    if (static_cast<VkSwapchainKHR>(swapchain) == nullptr ||
        static_cast<VkSemaphore>(waitSemaphore) == nullptr){
        return std::unexpected(renaru::Error::FramePresentationFailed);
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
            mOwner->noteError(renaru::Error::FramePresentationFailed);
            return std::unexpected(renaru::Error::FramePresentationFailed);
        }
    }catch(...){
        mOwner->noteError(renaru::Error::FramePresentationFailed);
        return std::unexpected(renaru::Error::FramePresentationFailed);
    }
    return {};
}
