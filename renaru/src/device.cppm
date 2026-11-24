module;

#include <renaru/vulkan.h>

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
    // Forward declaration: tipe lengkap ada di partisi masing-masing.
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
        public:
            using ResultType = std::expected<Device, Error>;

            static constexpr std::uint32_t MaxFramesInFlight = 2;

            // Didefinisikan di partisi lain (butuh tipe lengkap masing-masing).
            std::expected<Queue, Error> requestQueue(QueueFlags sflags) const;
            std::expected<Resource, Error> createImageResource(
                VkImage simage, VkFormat sformat, VkExtent2D sextent) const;
            std::expected<Resource, Error> createBufferResource(
                VkDeviceSize ssize, VkBufferUsageFlags susage, VkFormat sformat) const;
            std::expected<Swapchain, Error> createSwapchain(
                std::uint32_t swidth, std::uint32_t sheight) const;
            std::expected<std::vector<Resource>, Error> createSwapchainImages(
                const Swapchain& sswapchain) const;
            std::expected<RenderPass, Error> createRenderPass(
                const Swapchain& sswapchain) const;
            std::expected<Framebuffer, Error> createFramebuffer(
                const RenderPass& srenderPass, const Resource& simage) const;
            std::expected<CommandPool, Error> createCommandPool(const Queue& squeue) const;
            std::expected<std::vector<CommandBuffer>, Error> allocateCommandBuffers(
                const CommandPool& scommandPool, std::uint32_t scount) const;

            // Prototipe loop frame: wait fence -> acquire image -> jalankan
            // closure -> majukan frame_index. Fence/semaphore dan image index
            // disimpan di Device (lihat partisi :frame).
            template<typename Func>
            std::expected<void, Error> acquire(Func&& sfunc){
                if (auto started = beginFrame(); !started){
                    return std::unexpected(started.error());
                }

                try{
                    std::forward<Func>(sfunc)();
                }catch(...){
                    noteError(Error::CommandSubmissionFailed);
                }

                const Error serror = lastError();
                finishFrame();
                if (serror != Error::None){
                    return std::unexpected(serror);
                }
                return {};
            }

            // VkDevice dan objek sinkronisasi adalah raw handle: salinan tidak diizinkan.
            Device(const Device&) = delete;
            Device& operator=(const Device&) = delete;

            Device(Device&& sother) noexcept :
                sinstance(std::exchange(sother.sinstance, nullptr)),
                sphysicalDevice(std::exchange(sother.sphysicalDevice, nullptr)),
                ssurface(std::exchange(sother.ssurface, nullptr)),
                sdevice(std::exchange(sother.sdevice, nullptr)),
                squeueFamily(std::exchange(sother.squeueFamily, std::uint32_t{0})),
                sswapchain(std::exchange(sother.sswapchain, nullptr)),
                sframeExtent(std::exchange(sother.sframeExtent, VkExtent2D{})),
                sframebuffers(std::exchange(sother.sframebuffers, std::vector<VkFramebuffer>{})),
                simageAvailableSemaphores(std::exchange(sother.simageAvailableSemaphores, std::vector<VkSemaphore>{})),
                srenderFinishedSemaphores(std::exchange(sother.srenderFinishedSemaphores, std::vector<VkSemaphore>{})),
                sinFlightFences(std::exchange(sother.sinFlightFences, std::vector<VkFence>{})),
                sframeIndex(std::exchange(sother.sframeIndex, std::uint32_t{0})),
                scurrentImage(std::exchange(sother.scurrentImage, std::uint32_t{0})),
                sactiveCommandBuffer(std::exchange(sother.sactiveCommandBuffer, nullptr)),
                slastError(std::exchange(sother.slastError, Error::None))
            {}

            Device& operator=(Device&& sother) noexcept{
                if (this != &sother){
                    release();
                    sinstance = std::exchange(sother.sinstance, nullptr);
                    sphysicalDevice = std::exchange(sother.sphysicalDevice, nullptr);
                    ssurface = std::exchange(sother.ssurface, nullptr);
                    sdevice = std::exchange(sother.sdevice, nullptr);
                    squeueFamily = std::exchange(sother.squeueFamily, std::uint32_t{0});
                    sswapchain = std::exchange(sother.sswapchain, nullptr);
                    sframeExtent = std::exchange(sother.sframeExtent, VkExtent2D{});
                    sframebuffers = std::exchange(sother.sframebuffers, std::vector<VkFramebuffer>{});
                    simageAvailableSemaphores = std::exchange(sother.simageAvailableSemaphores, std::vector<VkSemaphore>{});
                    srenderFinishedSemaphores = std::exchange(sother.srenderFinishedSemaphores, std::vector<VkSemaphore>{});
                    sinFlightFences = std::exchange(sother.sinFlightFences, std::vector<VkFence>{});
                    sframeIndex = std::exchange(sother.sframeIndex, std::uint32_t{0});
                    scurrentImage = std::exchange(sother.scurrentImage, std::uint32_t{0});
                    sactiveCommandBuffer = std::exchange(sother.sactiveCommandBuffer, nullptr);
                    slastError = std::exchange(sother.slastError, Error::None);
                }
                return *this;
            }

            ~Device(){ release(); }

            VkDevice handle() const noexcept { return sdevice; }

            VkPhysicalDevice physicalDeviceHandle() const noexcept { return sphysicalDevice; }

            VkInstance instanceHandle() const noexcept { return sinstance; }

            VkSurfaceKHR surfaceHandle() const noexcept { return ssurface; }

            std::uint32_t queueFamilyIndex() const noexcept { return squeueFamily; }

            // Didefinisikan di :frame bersama state sinkronisasi.
            std::uint32_t currentImageIndex() const noexcept;
            std::uint32_t frameIndex() const noexcept;

            bool empty() const noexcept { return sdevice == nullptr; }

            // Menunggu semua kerja GPU selesai sebelum destruksi frame state.
            void waitIdle() const noexcept;

        private:
            friend class Adapter;
            friend class Queue;
            friend class RenderPass;
            friend class Framebuffer;
            friend class Swapchain;
            friend class CommandBuffer;

            Device(VkInstance sinstance, VkPhysicalDevice sphysicalDevice,
                   VkSurfaceKHR ssurface, VkDevice sdevice,
                   std::uint32_t squeueFamilyIndex) noexcept :
                sinstance(sinstance),
                sphysicalDevice(sphysicalDevice),
                ssurface(ssurface),
                sdevice(sdevice),
                squeueFamily(squeueFamilyIndex),
                sswapchain(nullptr),
                sframeExtent(),
                sframebuffers(),
                simageAvailableSemaphores(),
                srenderFinishedSemaphores(),
                sinFlightFences(),
                sframeIndex(0),
                scurrentImage(0),
                sactiveCommandBuffer(nullptr),
                slastError(Error::None)
            {}

            void release() noexcept{
                destroyFrameResources();
                if (sdevice != nullptr){
                    PFN_vkDestroyDevice pfnDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(
                        entry().resolveInstance(sinstance, "vkDestroyDevice"));
                    if (pfnDestroyDevice != nullptr){
                        pfnDestroyDevice(sdevice, nullptr);
                    }
                    sdevice = nullptr;
                }
                sinstance = nullptr;
                sphysicalDevice = nullptr;
                ssurface = nullptr;
                squeueFamily = 0;
                sswapchain = nullptr;
                sframeExtent = VkExtent2D{};
                sframebuffers.clear();
                sframeIndex = 0;
                scurrentImage = 0;
                sactiveCommandBuffer = nullptr;
                slastError = Error::None;
            }

            // Frame state. Bersifat mutable karena factory create* bersifat const
            // tetapi perlu mendaftarkan swapchain/framebuffer ke state ini.
            // Semua fungsi di bawah didefinisikan di partisi :frame.
            std::expected<void, Error> beginFrame() const;
            void finishFrame() const noexcept;
            std::expected<void, Error> ensureFrameResources() const;
            void destroyFrameResources() const noexcept;
            void registerSwapchain(VkSwapchainKHR snewSwapchain, VkExtent2D snewExtent,
                                   std::uint32_t snewImageCount) const noexcept;
            void registerFramebuffer(VkFramebuffer sframebuffer) const noexcept;
            VkSwapchainKHR swapchainHandle() const noexcept;
            VkFramebuffer framebufferForCurrentImage() const noexcept;
            VkExtent2D frameExtent() const noexcept;
            VkSemaphore imageAvailableSemaphore() const noexcept;
            VkSemaphore renderFinishedSemaphore() const noexcept;
            VkFence currentFence() const noexcept;
            VkCommandBuffer activeCommandBuffer() const noexcept;
            void setActiveCommandBuffer(VkCommandBuffer sbuffer) const noexcept;
            void clearActiveCommandBuffer() const noexcept;
            void noteError(Error serror) const noexcept;
            Error lastError() const noexcept;

            VkInstance sinstance = nullptr;
            VkPhysicalDevice sphysicalDevice = nullptr;
            VkSurfaceKHR ssurface = nullptr;
            VkDevice sdevice = nullptr;
            std::uint32_t squeueFamily = 0;

            mutable VkSwapchainKHR sswapchain = nullptr;
            mutable VkExtent2D sframeExtent{};
            mutable std::vector<VkFramebuffer> sframebuffers;
            mutable std::vector<VkSemaphore> simageAvailableSemaphores;
            mutable std::vector<VkSemaphore> srenderFinishedSemaphores;
            mutable std::vector<VkFence> sinFlightFences;
            mutable std::uint32_t sframeIndex = 0;
            mutable std::uint32_t scurrentImage = 0;
            mutable VkCommandBuffer sactiveCommandBuffer = nullptr;
            mutable Error slastError = Error::None;
    };
}

// Definisi lintas partisi: Adapter didefinisikan di :adapter, tetapi
// pembuatan Device membutuhkan tipe lengkap Device dari partisi ini.
std::expected<renaru::Device, renaru::Error> renaru::Adapter::createDevice(const renaru::Adapter& sadapter) const{
    if (empty() || sadapter.empty()){
        return std::unexpected(renaru::Error::DeviceCreationFailed);
    }

    const float spriority = 1.0F;
    VkDeviceQueueCreateInfo squeueInfo{};
    squeueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    squeueInfo.queueFamilyIndex = sadapter.squeueFamily;
    squeueInfo.queueCount = 1;
    squeueInfo.pQueuePriorities = &spriority;

    const char* sextensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo screateInfo{};
    screateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    screateInfo.queueCreateInfoCount = 1;
    screateInfo.pQueueCreateInfos = &squeueInfo;
    screateInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(sextensions));
    screateInfo.ppEnabledExtensionNames = sextensions;

    PFN_vkCreateDevice pfnCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
        entry().resolveInstance(sadapter.sinstance, "vkCreateDevice"));
    if (pfnCreateDevice == nullptr){
        return std::unexpected(renaru::Error::DeviceCreationFailed);
    }

    VkDevice screated = nullptr;
    if (pfnCreateDevice(sadapter.sphysicalDevice, &screateInfo, nullptr, &screated) != VK_SUCCESS ||
        screated == nullptr){
        return std::unexpected(renaru::Error::DeviceCreationFailed);
    }

    std::println("Aru RHI: Device(created!!)");

    return Device(sadapter.sinstance, sadapter.sphysicalDevice, sadapter.ssurface,
                  screated, sadapter.squeueFamily);
}
