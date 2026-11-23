module;

#include <renaru/vulkan.hpp>

#include <expected>
#include <limits>

export module renaru:frame;

import :device;

// State frame milik Device: fence/semaphore, frame_index, image index saat ini,
// serta register swapchain/framebuffer yang dipakai closure render.
// Focus di sini hanya plumbing Vulkan; pemanggilannya ada di acquire/record/
// render/submit/present sesuai prototipe di main.cpp.

std::expected<void, renaru::Error> renaru::Device::ensureFrameResources() const{
    if (this->empty() || static_cast<VkSwapchainKHR>(this->mSwapchain) == nullptr){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }
    if (this->mImageAvailableSemaphores.size() == MaxFramesInFlight &&
        this->mRenderFinishedSemaphores.size() == MaxFramesInFlight &&
        this->mInFlightFences.size() == MaxFramesInFlight){
        return {};
    }

    this->destroyFrameResources();

    try{
        for (std::uint32_t i = 0; i < MaxFramesInFlight; ++i){
            this->mImageAvailableSemaphores.push_back(
                this->mDevice.createSemaphore(vk::SemaphoreCreateInfo{}, nullptr,
                                              VULKAN_HPP_DEFAULT_DISPATCHER));
            this->mRenderFinishedSemaphores.push_back(
                this->mDevice.createSemaphore(vk::SemaphoreCreateInfo{}, nullptr,
                                              VULKAN_HPP_DEFAULT_DISPATCHER));

            vk::FenceCreateInfo fenceInfo;
            fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
            this->mInFlightFences.push_back(
                this->mDevice.createFence(fenceInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER));
        }
    }catch(...){
        this->destroyFrameResources();
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    if (this->mImageAvailableSemaphores.size() != MaxFramesInFlight ||
        this->mRenderFinishedSemaphores.size() != MaxFramesInFlight ||
        this->mInFlightFences.size() != MaxFramesInFlight){
        this->destroyFrameResources();
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    return {};
}

void renaru::Device::destroyFrameResources() const noexcept{
    const bool hasDevice = static_cast<VkDevice>(this->mDevice) != nullptr;

    if (hasDevice){
        for (const vk::Semaphore& semaphore : this->mImageAvailableSemaphores){
            if (static_cast<VkSemaphore>(semaphore) != nullptr){
                this->mDevice.destroySemaphore(semaphore, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
            }
        }
        for (const vk::Semaphore& semaphore : this->mRenderFinishedSemaphores){
            if (static_cast<VkSemaphore>(semaphore) != nullptr){
                this->mDevice.destroySemaphore(semaphore, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
            }
        }
        for (const vk::Fence& fence : this->mInFlightFences){
            if (static_cast<VkFence>(fence) != nullptr){
                this->mDevice.destroyFence(fence, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
            }
        }
    }

    this->mImageAvailableSemaphores.clear();
    this->mRenderFinishedSemaphores.clear();
    this->mInFlightFences.clear();
}

std::expected<void, renaru::Error> renaru::Device::beginFrame() const{
    if (auto ready = this->ensureFrameResources(); !ready){
        return ready;
    }
    if (this->mFrameIndex >= this->mInFlightFences.size()){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    this->mLastError = renaru::Error::None;
    this->mActiveCommandBuffer = vk::CommandBuffer{};

    const vk::Fence fence = this->mInFlightFences[this->mFrameIndex];
    const std::uint64_t timeout = (std::numeric_limits<std::uint64_t>::max)();

    try{
        const vk::Result waited = this->mDevice.waitForFences(
            1, &fence, VK_TRUE, timeout, VULKAN_HPP_DEFAULT_DISPATCHER);
        if (waited != vk::Result::eSuccess){
            return std::unexpected(renaru::Error::FrameAcquisitionFailed);
        }

        const vk::Result reset = this->mDevice.resetFences(
            1, &fence, VULKAN_HPP_DEFAULT_DISPATCHER);
        if (reset != vk::Result::eSuccess){
            return std::unexpected(renaru::Error::FrameAcquisitionFailed);
        }

        const vk::Result acquired = this->mDevice.acquireNextImageKHR(
            this->mSwapchain, timeout,
            this->mImageAvailableSemaphores[this->mFrameIndex],
            vk::Fence{}, &this->mCurrentImage, VULKAN_HPP_DEFAULT_DISPATCHER);
        if (acquired != vk::Result::eSuccess &&
            acquired != vk::Result::eSuboptimalKHR){
            return std::unexpected(renaru::Error::FrameAcquisitionFailed);
        }
    }catch(...){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    return {};
}

void renaru::Device::waitIdle() const noexcept{
    if (this->empty()){
        return;
    }
    try{
        this->mDevice.waitIdle(VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        // Tidak ada aksi recovery di jalur destruksi.
    }
}

void renaru::Device::finishFrame() const noexcept{
    if (!this->mInFlightFences.empty()){
        this->mFrameIndex = (this->mFrameIndex + 1) %
                            static_cast<std::uint32_t>(this->mInFlightFences.size());
    }
    this->mActiveCommandBuffer = vk::CommandBuffer{};
}

void renaru::Device::registerSwapchain(vk::SwapchainKHR swapchain,
                                         vk::Extent2D extent,
                                         std::uint32_t imageCount) const noexcept{
    this->mSwapchain = swapchain;
    this->mFrameExtent = extent;
    this->mFramebuffers.clear();
    this->mFramebuffers.reserve(imageCount);
    this->mFrameIndex = 0;
    this->mCurrentImage = 0;
    this->destroyFrameResources();
}

void renaru::Device::registerFramebuffer(vk::Framebuffer framebuffer) const noexcept{
    if (static_cast<VkFramebuffer>(framebuffer) != nullptr){
        this->mFramebuffers.push_back(framebuffer);
    }
}

vk::SwapchainKHR renaru::Device::swapchainHandle() const noexcept{
    return this->mSwapchain;
}

vk::Framebuffer renaru::Device::framebufferForCurrentImage() const noexcept{
    if (this->mCurrentImage >= this->mFramebuffers.size()){
        return vk::Framebuffer{};
    }
    return this->mFramebuffers[this->mCurrentImage];
}

vk::Extent2D renaru::Device::frameExtent() const noexcept{
    return this->mFrameExtent;
}

vk::Semaphore renaru::Device::imageAvailableSemaphore() const noexcept{
    if (this->mFrameIndex >= this->mImageAvailableSemaphores.size()){
        return vk::Semaphore{};
    }
    return this->mImageAvailableSemaphores[this->mFrameIndex];
}

vk::Semaphore renaru::Device::renderFinishedSemaphore() const noexcept{
    if (this->mFrameIndex >= this->mRenderFinishedSemaphores.size()){
        return vk::Semaphore{};
    }
    return this->mRenderFinishedSemaphores[this->mFrameIndex];
}

vk::Fence renaru::Device::currentFence() const noexcept{
    if (this->mFrameIndex >= this->mInFlightFences.size()){
        return vk::Fence{};
    }
    return this->mInFlightFences[this->mFrameIndex];
}

std::uint32_t renaru::Device::currentImageIndex() const noexcept{
    return this->mCurrentImage;
}

std::uint32_t renaru::Device::frameIndex() const noexcept{
    return this->mFrameIndex;
}

vk::CommandBuffer renaru::Device::activeCommandBuffer() const noexcept{
    return this->mActiveCommandBuffer;
}

void renaru::Device::setActiveCommandBuffer(vk::CommandBuffer buffer) const noexcept{
    this->mActiveCommandBuffer = buffer;
}

void renaru::Device::clearActiveCommandBuffer() const noexcept{
    this->mActiveCommandBuffer = vk::CommandBuffer{};
}

void renaru::Device::noteError(renaru::Error error) const noexcept{
    if (this->mLastError == renaru::Error::None){
        this->mLastError = error;
    }
}

renaru::Error renaru::Device::lastError() const noexcept{
    return this->mLastError;
}
