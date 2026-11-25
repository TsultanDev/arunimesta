module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <iterator>
#include <print>
#include <utility>
#include <vector>

export module renaru:device;

import :entry;
import :instance;
import :adapter;

export namespace renaru {
    class Queue;
    class Resource;
    class Swapchain;
    class RenderPass;
    class Framebuffer;
    class CommandPool;
    class CommandBuffer;
}

export namespace renaru {
    class Device {
        private:
            using ResultType = std::expected<Device, Error>;

            VkInstance sInstance = nullptr;
            VkPhysicalDevice sPhysicalDevice = nullptr;
            VkSurfaceKHR sSurface = nullptr;
            VkDevice sDevice = nullptr;
            std::uint32_t sQueueFamily = 0;

            // Frame state ditulis factory const, jadi mutable.
            mutable VkSwapchainKHR sSwapchain = nullptr;
            mutable VkExtent2D sFrameExtent{};
            mutable std::vector<VkFramebuffer> sFramebuffers{};
            mutable std::vector<VkSemaphore> sImageAvailableSemaphores{};
            mutable std::vector<VkSemaphore> sRenderFinishedSemaphores{};
            mutable std::vector<VkFence> sInFlightFences{};
            mutable std::uint32_t sFrameIndex = 0;
            mutable std::uint32_t sCurrentImage = 0;
            mutable VkCommandBuffer sActiveCommandBuffer = nullptr;
            mutable Error sLastError = Error::eNone;

            Device(VkInstance sInstance, VkPhysicalDevice sPhysicalDevice,
                   VkSurfaceKHR sSurface, VkDevice sDevice,
                   std::uint32_t sQueueFamilyIndex) noexcept :
                sInstance(sInstance),
                sPhysicalDevice(sPhysicalDevice),
                sSurface(sSurface),
                sDevice(sDevice),
                sQueueFamily(sQueueFamilyIndex)
            {}

            void sDestroy() noexcept{
                sDestroyFrameResources();
                if (sDevice != nullptr){
                    PFN_vkDestroyDevice pfnDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(
                        entry().resolveInstance(sInstance, "vkDestroyDevice"));
                    if (pfnDestroyDevice != nullptr){
                        pfnDestroyDevice(sDevice, nullptr);
                    }
                    sDevice = nullptr;
                }
                sInstance = nullptr;
                sPhysicalDevice = nullptr;
                sSurface = nullptr;
                sQueueFamily = 0;
                sSwapchain = nullptr;
                sFrameExtent = VkExtent2D{};
                sFramebuffers.clear();
                sFrameIndex = 0;
                sCurrentImage = 0;
                sActiveCommandBuffer = nullptr;
                sLastError = Error::eNone;
            }

            // Fungsi internal frame didefinisikan di partisi :frame.
            std::expected<void, Error> sBeginFrame() const;
            void sFinishFrame() const noexcept;
            std::expected<void, Error> sEnsureFrameResources() const;
            void sDestroyFrameResources() const noexcept;
            void sRegisterSwapchain(VkSwapchainKHR sNewSwapchain, VkExtent2D sNewExtent,
                                    std::uint32_t sNewImageCount) const noexcept;
            void sRegisterFramebuffer(VkFramebuffer sNewFramebuffer) const noexcept;
            VkFramebuffer sFramebufferForCurrentImage() const noexcept;
            VkExtent2D sCurrentFrameExtent() const noexcept;
            VkSemaphore sImageAvailableSemaphore() const noexcept;
            VkSemaphore sRenderFinishedSemaphore() const noexcept;
            VkFence sCurrentFence() const noexcept;
            void sNoteError(Error sError) const noexcept;
            Error sCurrentError() const noexcept;

            friend class Adapter;
            friend class Queue;
            friend class RenderPass;
            friend class Framebuffer;
            friend class Swapchain;
            friend class CommandPool;
            friend class CommandBuffer;

        public:
            static constexpr std::uint32_t MaxFramesInFlight = 2;

            ~Device(){ sDestroy(); }

            Device(const Device&) = delete;
            Device& operator=(const Device&) = delete;

            Device(Device&& sOther) noexcept :
                sInstance(std::exchange(sOther.sInstance, nullptr)),
                sPhysicalDevice(std::exchange(sOther.sPhysicalDevice, nullptr)),
                sSurface(std::exchange(sOther.sSurface, nullptr)),
                sDevice(std::exchange(sOther.sDevice, nullptr)),
                sQueueFamily(std::exchange(sOther.sQueueFamily, std::uint32_t{0})),
                sSwapchain(std::exchange(sOther.sSwapchain, nullptr)),
                sFrameExtent(std::exchange(sOther.sFrameExtent, VkExtent2D{})),
                sFramebuffers(std::exchange(sOther.sFramebuffers, std::vector<VkFramebuffer>{})),
                sImageAvailableSemaphores(std::exchange(sOther.sImageAvailableSemaphores, std::vector<VkSemaphore>{})),
                sRenderFinishedSemaphores(std::exchange(sOther.sRenderFinishedSemaphores, std::vector<VkSemaphore>{})),
                sInFlightFences(std::exchange(sOther.sInFlightFences, std::vector<VkFence>{})),
                sFrameIndex(std::exchange(sOther.sFrameIndex, std::uint32_t{0})),
                sCurrentImage(std::exchange(sOther.sCurrentImage, std::uint32_t{0})),
                sActiveCommandBuffer(std::exchange(sOther.sActiveCommandBuffer, nullptr)),
                sLastError(std::exchange(sOther.sLastError, Error::eNone))
            {}

            Device& operator=(Device&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sInstance = std::exchange(sOther.sInstance, nullptr);
                    sPhysicalDevice = std::exchange(sOther.sPhysicalDevice, nullptr);
                    sSurface = std::exchange(sOther.sSurface, nullptr);
                    sDevice = std::exchange(sOther.sDevice, nullptr);
                    sQueueFamily = std::exchange(sOther.sQueueFamily, std::uint32_t{0});
                    sSwapchain = std::exchange(sOther.sSwapchain, nullptr);
                    sFrameExtent = std::exchange(sOther.sFrameExtent, VkExtent2D{});
                    sFramebuffers = std::exchange(sOther.sFramebuffers, std::vector<VkFramebuffer>{});
                    sImageAvailableSemaphores = std::exchange(sOther.sImageAvailableSemaphores, std::vector<VkSemaphore>{});
                    sRenderFinishedSemaphores = std::exchange(sOther.sRenderFinishedSemaphores, std::vector<VkSemaphore>{});
                    sInFlightFences = std::exchange(sOther.sInFlightFences, std::vector<VkFence>{});
                    sFrameIndex = std::exchange(sOther.sFrameIndex, std::uint32_t{0});
                    sCurrentImage = std::exchange(sOther.sCurrentImage, std::uint32_t{0});
                    sActiveCommandBuffer = std::exchange(sOther.sActiveCommandBuffer, nullptr);
                    sLastError = std::exchange(sOther.sLastError, Error::eNone);
                }
                return *this;
            }

            std::expected<Queue, Error> requestQueue(QueueFlags sFlags) const;
            std::expected<Resource, Error> createImageResource(
                VkImage sImage, VkFormat sFormat, VkExtent2D sExtent) const;
            std::expected<Resource, Error> createBufferResource(
                VkDeviceSize sSize, VkBufferUsageFlags sUsage, VkFormat sFormat) const;
            std::expected<Swapchain, Error> createSwapchain(
                std::uint32_t sWidth, std::uint32_t sHeight) const;
            std::expected<std::vector<Resource>, Error> createSwapchainImages(
                const Swapchain& sSwapchain) const;
            std::expected<RenderPass, Error> createRenderPass(
                const Swapchain& sSwapchain) const;
            std::expected<Framebuffer, Error> createFramebuffer(
                const RenderPass& sRenderPass, const Resource& sImage) const;
            std::expected<CommandPool, Error> createCommandPool(const Queue& sQueue) const;
            std::expected<std::vector<CommandBuffer>, Error> allocateCommandBuffers(
                const CommandPool& sCommandPool, std::uint32_t sCount) const;

            template<typename Func>
            std::expected<void, Error> acquire(Func&& sFunc){
                if (auto sStarted = sBeginFrame(); !sStarted){
                    return std::unexpected(sStarted.error());
                }

                try{
                    std::forward<Func>(sFunc)();
                }catch(...){
                    sNoteError(Error::eCommandSubmissionFailed);
                }

                const Error sError = sCurrentError();
                sFinishFrame();
                if (sError != Error::eNone){
                    return std::unexpected(sError);
                }
                return {};
            }

            // Dipakai loop utama untuk memilih command buffer per swapchain image.
            std::uint32_t currentImageIndex() const noexcept{ return sCurrentImage; }

            void waitIdle() const noexcept;
    };
}

// Adapter::createDevice() didefinisikan di sini karena butuh tipe lengkap Device.
std::expected<renaru::Device, renaru::Error>
renaru::Adapter::createDevice(const renaru::Adapter& sAdapter) const{
    if (sPhysicalDevice == nullptr || sAdapter.sPhysicalDevice == nullptr){
        return std::unexpected(renaru::Error::eDeviceCreationFailed);
    }

    const float sPriority = 1.0F;
    VkDeviceQueueCreateInfo sQueueInfo{};
    sQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    sQueueInfo.queueFamilyIndex = sAdapter.sQueueFamily;
    sQueueInfo.queueCount = 1;
    sQueueInfo.pQueuePriorities = &sPriority;

    const char* sExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo sCreateInfo{};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    sCreateInfo.queueCreateInfoCount = 1;
    sCreateInfo.pQueueCreateInfos = &sQueueInfo;
    sCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(sExtensions));
    sCreateInfo.ppEnabledExtensionNames = sExtensions;

    PFN_vkCreateDevice pfnCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
        entry().resolveInstance(sAdapter.sInstance, "vkCreateDevice"));
    if (pfnCreateDevice == nullptr){
        return std::unexpected(renaru::Error::eDeviceCreationFailed);
    }

    VkDevice sCreated = nullptr;
    if (pfnCreateDevice(sAdapter.sPhysicalDevice, &sCreateInfo, nullptr, &sCreated) != VK_SUCCESS ||
        sCreated == nullptr){
        return std::unexpected(renaru::Error::eDeviceCreationFailed);
    }

    std::println("Aru RHI: Device(created!!)");

    return Device(sAdapter.sInstance, sAdapter.sPhysicalDevice, sAdapter.sSurface,
                  sCreated, sAdapter.sQueueFamily);
}
