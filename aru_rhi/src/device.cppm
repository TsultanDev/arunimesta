module;

#include <aru_rhi/vulkan.hpp>

#include <expected>
#include <vector>
#include <print>

#include <cstdint>
#include <iterator>
#include <utility>

export module aru.rhi:device;

import :instance;
import :adapter;

export namespace aru::rhi {
    // Forward declaration: tipe lengkap ada di partisi masing-masing.
    class Queue;
    class Resource;
    class Swapchain;
    class RenderPass;
    class Framebuffer;
    class CommandPool;
    class CommandBuffer;
}

export namespace aru::rhi {
    class Device {
        public:
            using ResultType = std::expected<Device, Error>;

            static constexpr std::uint32_t MaxFramesInFlight = 2;

            // Didefinisikan di partisi lain (butuh tipe lengkap masing-masing).
            std::expected<Queue, Error> requestQueue(QueueFlags flags) const;
            std::expected<Resource, Error> createImageResource(
                vk::Image image, vk::Format format, vk::Extent2D extent) const;
            std::expected<Resource, Error> createBufferResource(
                vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Format format) const;
            std::expected<Swapchain, Error> createSwapchain(
                std::uint32_t width, std::uint32_t height) const;
            std::expected<std::vector<Resource>, Error> createSwapchainImages(
                const Swapchain& swapchain) const;
            std::expected<RenderPass, Error> createRenderPass(
                const Swapchain& swapchain) const;
            std::expected<Framebuffer, Error> createFramebuffer(
                const RenderPass& renderPass, const Resource& image) const;
            std::expected<CommandPool, Error> createCommandPool(const Queue& queue) const;
            std::expected<std::vector<CommandBuffer>, Error> allocateCommandBuffers(
                const CommandPool& commandPool, std::uint32_t count) const;

            // Prototipe loop frame: wait fence -> acquire image -> jalankan
            // closure -> majukan frame_index. Fence/semaphore dan image index
            // disimpan di Device (lihat partisi :frame).
            template<typename Func>
            std::expected<void, Error> acquire(Func&& func){
                if (auto started = this->beginFrame(); !started){
                    return std::unexpected(started.error());
                }

                try{
                    std::forward<Func>(func)();
                }catch(...){
                    this->noteError(Error::CommandSubmissionFailed);
                }

                const Error error = this->lastError();
                this->finishFrame();
                if (error != Error::None){
                    return std::unexpected(error);
                }
                return {};
            }

            // vk::Device dan objek sinkronisasi adalah raw handle: salinan tidak diizinkan.
            Device(const Device&) = delete;
            Device& operator=(const Device&) = delete;

            Device(Device&& other) noexcept :
                mInstance(std::exchange(other.mInstance, vk::Instance{})),
                mPhysicalDevice(std::exchange(other.mPhysicalDevice, vk::PhysicalDevice{})),
                mSurface(std::exchange(other.mSurface, vk::SurfaceKHR{})),
                mDevice(std::exchange(other.mDevice, vk::Device{})),
                mQueueFamily(std::exchange(other.mQueueFamily, std::uint32_t{0})),
                mSwapchain(std::exchange(other.mSwapchain, vk::SwapchainKHR{})),
                mFrameExtent(std::exchange(other.mFrameExtent, vk::Extent2D{})),
                mFramebuffers(std::exchange(other.mFramebuffers, std::vector<vk::Framebuffer>{})),
                mImageAvailableSemaphores(std::exchange(other.mImageAvailableSemaphores, std::vector<vk::Semaphore>{})),
                mRenderFinishedSemaphores(std::exchange(other.mRenderFinishedSemaphores, std::vector<vk::Semaphore>{})),
                mInFlightFences(std::exchange(other.mInFlightFences, std::vector<vk::Fence>{})),
                mFrameIndex(std::exchange(other.mFrameIndex, std::uint32_t{0})),
                mCurrentImage(std::exchange(other.mCurrentImage, std::uint32_t{0})),
                mActiveCommandBuffer(std::exchange(other.mActiveCommandBuffer, vk::CommandBuffer{})),
                mLastError(std::exchange(other.mLastError, Error::None))
            {}

            Device& operator=(Device&& other) noexcept{
                if (this != &other){
                    this->release();
                    this->mInstance = std::exchange(other.mInstance, vk::Instance{});
                    this->mPhysicalDevice = std::exchange(other.mPhysicalDevice, vk::PhysicalDevice{});
                    this->mSurface = std::exchange(other.mSurface, vk::SurfaceKHR{});
                    this->mDevice = std::exchange(other.mDevice, vk::Device{});
                    this->mQueueFamily = std::exchange(other.mQueueFamily, std::uint32_t{0});
                    this->mSwapchain = std::exchange(other.mSwapchain, vk::SwapchainKHR{});
                    this->mFrameExtent = std::exchange(other.mFrameExtent, vk::Extent2D{});
                    this->mFramebuffers = std::exchange(other.mFramebuffers, std::vector<vk::Framebuffer>{});
                    this->mImageAvailableSemaphores = std::exchange(other.mImageAvailableSemaphores, std::vector<vk::Semaphore>{});
                    this->mRenderFinishedSemaphores = std::exchange(other.mRenderFinishedSemaphores, std::vector<vk::Semaphore>{});
                    this->mInFlightFences = std::exchange(other.mInFlightFences, std::vector<vk::Fence>{});
                    this->mFrameIndex = std::exchange(other.mFrameIndex, std::uint32_t{0});
                    this->mCurrentImage = std::exchange(other.mCurrentImage, std::uint32_t{0});
                    this->mActiveCommandBuffer = std::exchange(other.mActiveCommandBuffer, vk::CommandBuffer{});
                    this->mLastError = std::exchange(other.mLastError, Error::None);
                }
                return *this;
            }

            ~Device(){ this->release(); }

            vk::Device handle() const noexcept { return this->mDevice; }

            vk::PhysicalDevice physicalDeviceHandle() const noexcept { return this->mPhysicalDevice; }

            vk::Instance instanceHandle() const noexcept { return this->mInstance; }

            vk::SurfaceKHR surfaceHandle() const noexcept { return this->mSurface; }

            std::uint32_t queueFamilyIndex() const noexcept { return this->mQueueFamily; }

            // Didefinisikan di :frame bersama state sinkronisasi.
            std::uint32_t currentImageIndex() const noexcept;
            std::uint32_t frameIndex() const noexcept;

            bool empty() const noexcept { return static_cast<VkDevice>(this->mDevice) == nullptr; }

            // Menunggu semua kerja GPU selesai sebelum destruksi frame state.
            void waitIdle() const noexcept;

        private:
            friend class Adapter;
            friend class Queue;
            friend class RenderPass;
            friend class Framebuffer;
            friend class Swapchain;
            friend class CommandBuffer;

            Device(vk::Instance instance, vk::PhysicalDevice physicalDevice,
                   vk::SurfaceKHR surface, vk::Device device,
                   std::uint32_t queueFamilyIndex) noexcept :
                mInstance(instance),
                mPhysicalDevice(physicalDevice),
                mSurface(surface),
                mDevice(device),
                mQueueFamily(queueFamilyIndex),
                mSwapchain(),
                mFrameExtent(),
                mFramebuffers(),
                mImageAvailableSemaphores(),
                mRenderFinishedSemaphores(),
                mInFlightFences(),
                mFrameIndex(0),
                mCurrentImage(0),
                mActiveCommandBuffer(),
                mLastError(Error::None)
            {}

            void release() noexcept{
                this->destroyFrameResources();
                if (static_cast<VkDevice>(this->mDevice) != nullptr){
                    this->mDevice.destroy(nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
                    this->mDevice = vk::Device{};
                }
                this->mPhysicalDevice = vk::PhysicalDevice{};
                this->mSurface = vk::SurfaceKHR{};
                this->mQueueFamily = 0;
                this->mSwapchain = vk::SwapchainKHR{};
                this->mFrameExtent = vk::Extent2D{};
                this->mFramebuffers.clear();
                this->mFrameIndex = 0;
                this->mCurrentImage = 0;
                this->mActiveCommandBuffer = vk::CommandBuffer{};
                this->mLastError = Error::None;
            }

            // Frame state. Bersifat mutable karena factory create* bersifat const
            // tetapi perlu mendaftarkan swapchain/framebuffer ke state ini.
            // Semua fungsi di bawah didefinisikan di partisi :frame.
            std::expected<void, Error> beginFrame() const;
            void finishFrame() const noexcept;
            std::expected<void, Error> ensureFrameResources() const;
            void destroyFrameResources() const noexcept;
            void registerSwapchain(vk::SwapchainKHR swapchain, vk::Extent2D extent,
                                   std::uint32_t imageCount) const noexcept;
            void registerFramebuffer(vk::Framebuffer framebuffer) const noexcept;
            vk::SwapchainKHR swapchainHandle() const noexcept;
            vk::Framebuffer framebufferForCurrentImage() const noexcept;
            vk::Extent2D frameExtent() const noexcept;
            vk::Semaphore imageAvailableSemaphore() const noexcept;
            vk::Semaphore renderFinishedSemaphore() const noexcept;
            vk::Fence currentFence() const noexcept;
            vk::CommandBuffer activeCommandBuffer() const noexcept;
            void setActiveCommandBuffer(vk::CommandBuffer buffer) const noexcept;
            void clearActiveCommandBuffer() const noexcept;
            void noteError(Error error) const noexcept;
            Error lastError() const noexcept;

            vk::Instance mInstance;
            vk::PhysicalDevice mPhysicalDevice;
            vk::SurfaceKHR mSurface;
            vk::Device mDevice;
            std::uint32_t mQueueFamily;

            mutable vk::SwapchainKHR mSwapchain;
            mutable vk::Extent2D mFrameExtent;
            mutable std::vector<vk::Framebuffer> mFramebuffers;
            mutable std::vector<vk::Semaphore> mImageAvailableSemaphores;
            mutable std::vector<vk::Semaphore> mRenderFinishedSemaphores;
            mutable std::vector<vk::Fence> mInFlightFences;
            mutable std::uint32_t mFrameIndex;
            mutable std::uint32_t mCurrentImage;
            mutable vk::CommandBuffer mActiveCommandBuffer;
            mutable Error mLastError;
    };
}

// Definisi lintas partisi: Adapter didefinisikan di :adapter, tetapi
// pembuatan Device membutuhkan tipe lengkap Device dari partisi ini.
std::expected<aru::rhi::Device, aru::rhi::Error> aru::rhi::Adapter::createDevice(const aru::rhi::Adapter& adapter) const{
    if (this->empty() || adapter.empty()){
        return std::unexpected(aru::rhi::Error::DeviceCreationFailed);
    }

    const float priority = 1.0F;
    vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.setQueueFamilyIndex(adapter.mQueueFamily);
    queueInfo.setQueueCount(1);
    queueInfo.setPQueuePriorities(&priority);

    const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    vk::DeviceCreateInfo createInfo;
    createInfo.setQueueCreateInfoCount(1);
    createInfo.setPQueueCreateInfos(&queueInfo);
    createInfo.setEnabledExtensionCount(static_cast<std::uint32_t>(std::size(extensions)));
    createInfo.setPpEnabledExtensionNames(extensions);

    vk::Device created;
    try{
        created = adapter.mPhysicalDevice.createDevice(createInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(aru::rhi::Error::DeviceCreationFailed);
    }

    std::println("Aru RHI: Device(created!!)");

    return Device(adapter.mInstance, adapter.mPhysicalDevice, adapter.mSurface,
                  created, adapter.mQueueFamily);
}
