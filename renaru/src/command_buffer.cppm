module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <print>
#include <utility>
#include <vector>

export module renaru:command_buffer;

import :entry;
import :device;
import :command_pool;
import :queue;

export namespace renaru {
    class CommandBuffer {
        private:
            using ResultType = std::expected<CommandBuffer, Error>;

            const Device* sOwner = nullptr;
            VkDevice sDevice = nullptr;
            VkCommandPool sPool = nullptr;
            VkCommandBuffer sBuffer = nullptr;

            CommandBuffer(const Device* sOwner, VkDevice sDevice, VkCommandPool sPool,
                          VkCommandBuffer sBuffer) noexcept :
                sOwner(sOwner),
                sDevice(sDevice),
                sPool(sPool),
                sBuffer(sBuffer)
            {}

            void sDestroy() noexcept{
                if (sBuffer != nullptr && sDevice != nullptr && sPool != nullptr){
                    PFN_vkFreeCommandBuffers pfnFreeCommandBuffers =
                        reinterpret_cast<PFN_vkFreeCommandBuffers>(
                            entry().resolveDevice(sDevice, "vkFreeCommandBuffers"));
                    if (pfnFreeCommandBuffers != nullptr){
                        pfnFreeCommandBuffers(sDevice, sPool, 1, &sBuffer);
                    }
                    sBuffer = nullptr;
                }
                sOwner = nullptr;
                sDevice = nullptr;
                sPool = nullptr;
            }

            std::expected<void, Error> sBeginRecording(){
                if (sBuffer == nullptr || sOwner == nullptr || sOwner->sDevice == nullptr){
                    return std::unexpected(renaru::Error::eCommandBufferRecordingFailed);
                }

                // Pool dibuat dengan RESET_COMMAND_BUFFER, jadi reset diperbolehkan.
                (void)reset();
                if (!begin()){
                    return std::unexpected(renaru::Error::eCommandBufferRecordingFailed);
                }

                sOwner->sActiveCommandBuffer = sBuffer;
                return {};
            }

            void sEndRecording() noexcept{
                if (sOwner != nullptr){
                    sOwner->sActiveCommandBuffer = nullptr;
                }
                end();
            }

            friend class Device;
            friend class Queue;

        public:
            ~CommandBuffer(){ sDestroy(); }

            CommandBuffer(const CommandBuffer&) = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;

            CommandBuffer(CommandBuffer&& sOther) noexcept :
                sOwner(std::exchange(sOther.sOwner, nullptr)),
                sDevice(std::exchange(sOther.sDevice, nullptr)),
                sPool(std::exchange(sOther.sPool, nullptr)),
                sBuffer(std::exchange(sOther.sBuffer, nullptr))
            {}

            CommandBuffer& operator=(CommandBuffer&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sOwner = std::exchange(sOther.sOwner, nullptr);
                    sDevice = std::exchange(sOther.sDevice, nullptr);
                    sPool = std::exchange(sOther.sPool, nullptr);
                    sBuffer = std::exchange(sOther.sBuffer, nullptr);
                }
                return *this;
            }

            template<typename Func>
            std::expected<void, Error> record(Func&& sFunc){
                if (auto sStarted = sBeginRecording(); !sStarted){
                    return std::unexpected(sStarted.error());
                }

                try{
                    std::forward<Func>(sFunc)();
                }catch(...){
                    sEndRecording();
                    const renaru::Error sError = renaru::Error::eCommandBufferRecordingFailed;
                    if (sOwner != nullptr){
                        sOwner->sNoteError(sError);
                    }
                    return std::unexpected(sError);
                }

                sEndRecording();
                if (sOwner != nullptr && sOwner->sCurrentError() != Error::eNone){
                    return std::unexpected(sOwner->sCurrentError());
                }
                return {};
            }

            bool begin(VkCommandBufferUsageFlags sFlags = 0) noexcept{
                if (sBuffer == nullptr || sDevice == nullptr){
                    return false;
                }
                PFN_vkBeginCommandBuffer pfnBeginCommandBuffer =
                    reinterpret_cast<PFN_vkBeginCommandBuffer>(
                        entry().resolveDevice(sDevice, "vkBeginCommandBuffer"));
                if (pfnBeginCommandBuffer == nullptr){
                    return false;
                }

                VkCommandBufferBeginInfo sInfo{};
                sInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                sInfo.flags = sFlags;
                return pfnBeginCommandBuffer(sBuffer, &sInfo) == VK_SUCCESS;
            }

            void end() noexcept{
                if (sBuffer == nullptr || sDevice == nullptr){
                    return;
                }
                PFN_vkEndCommandBuffer pfnEndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(
                    entry().resolveDevice(sDevice, "vkEndCommandBuffer"));
                if (pfnEndCommandBuffer != nullptr){
                    pfnEndCommandBuffer(sBuffer);
                }
            }

            bool reset(VkCommandBufferResetFlags sFlags = 0) noexcept{
                if (sBuffer == nullptr || sDevice == nullptr){
                    return false;
                }
                PFN_vkResetCommandBuffer pfnResetCommandBuffer =
                    reinterpret_cast<PFN_vkResetCommandBuffer>(
                        entry().resolveDevice(sDevice, "vkResetCommandBuffer"));
                if (pfnResetCommandBuffer == nullptr){
                    return false;
                }
                return pfnResetCommandBuffer(sBuffer, sFlags) == VK_SUCCESS;
            }
    };
}

std::expected<std::vector<renaru::CommandBuffer>, renaru::Error>
renaru::Device::allocateCommandBuffers(const renaru::CommandPool& sCommandPool,
                                       std::uint32_t sCount) const{
    if (sDevice == nullptr || sCommandPool.sPool == nullptr || sCount == 0){
        return std::unexpected(renaru::Error::eCommandBufferAllocationFailed);
    }
    if (sCommandPool.sDevice != sDevice){
        return std::unexpected(renaru::Error::eCommandBufferAllocationFailed);
    }

    PFN_vkAllocateCommandBuffers pfnAllocateCommandBuffers =
        reinterpret_cast<PFN_vkAllocateCommandBuffers>(
            entry().resolveDevice(sDevice, "vkAllocateCommandBuffers"));
    PFN_vkFreeCommandBuffers pfnFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(
        entry().resolveDevice(sDevice, "vkFreeCommandBuffers"));
    if (pfnAllocateCommandBuffers == nullptr || pfnFreeCommandBuffers == nullptr){
        return std::unexpected(renaru::Error::eCommandBufferAllocationFailed);
    }

    VkCommandBufferAllocateInfo sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    sInfo.commandPool = sCommandPool.sPool;
    sInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    sInfo.commandBufferCount = sCount;

    std::vector<VkCommandBuffer> sAllocated(sCount, nullptr);
    if (pfnAllocateCommandBuffers(sDevice, &sInfo, sAllocated.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::eCommandBufferAllocationFailed);
    }

    for (const VkCommandBuffer sBuffer : sAllocated){
        if (sBuffer == nullptr){
            for (const VkCommandBuffer sCreated : sAllocated){
                if (sCreated != nullptr){
                    pfnFreeCommandBuffers(sDevice, sCommandPool.sPool, 1, &sCreated);
                }
            }
            return std::unexpected(renaru::Error::eCommandBufferAllocationFailed);
        }
    }

    std::vector<CommandBuffer> sResult;
    sResult.reserve(sAllocated.size());
    for (const VkCommandBuffer sBuffer : sAllocated){
        sResult.push_back(CommandBuffer(this, sDevice, sCommandPool.sPool, sBuffer));
    }

    std::println("Aru RHI: CommandBuffers(allocated!!) count={}", sResult.size());

    return sResult;
}

std::expected<void, renaru::Error> renaru::Queue::submit(
    const renaru::CommandBuffer& sBuffer) const{
    if (sQueue == nullptr || sOwner == nullptr || sOwner->sDevice == nullptr ||
        sBuffer.sBuffer == nullptr){
        return std::unexpected(renaru::Error::eCommandSubmissionFailed);
    }
    if (sBuffer.sDevice != sOwner->sDevice){
        return std::unexpected(renaru::Error::eCommandSubmissionFailed);
    }

    const VkCommandBuffer sCommandBuffer = sBuffer.sBuffer;
    const VkSemaphore sWaitSemaphore = sOwner->sImageAvailableSemaphore();
    const VkSemaphore sSignalSemaphore = sOwner->sRenderFinishedSemaphore();
    const VkFence sFence = sOwner->sCurrentFence();
    if (sWaitSemaphore == nullptr || sSignalSemaphore == nullptr || sFence == nullptr){
        return std::unexpected(renaru::Error::eCommandSubmissionFailed);
    }

    const VkPipelineStageFlags sWaitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    sInfo.waitSemaphoreCount = 1;
    sInfo.pWaitSemaphores = &sWaitSemaphore;
    sInfo.pWaitDstStageMask = sWaitStages;
    sInfo.commandBufferCount = 1;
    sInfo.pCommandBuffers = &sCommandBuffer;
    sInfo.signalSemaphoreCount = 1;
    sInfo.pSignalSemaphores = &sSignalSemaphore;

    PFN_vkQueueSubmit pfnQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(
        entry().resolveDevice(sDevice, "vkQueueSubmit"));
    if (pfnQueueSubmit == nullptr){
        sOwner->sNoteError(renaru::Error::eCommandSubmissionFailed);
        return std::unexpected(renaru::Error::eCommandSubmissionFailed);
    }
    if (pfnQueueSubmit(sQueue, 1, &sInfo, sFence) != VK_SUCCESS){
        sOwner->sNoteError(renaru::Error::eCommandSubmissionFailed);
        return std::unexpected(renaru::Error::eCommandSubmissionFailed);
    }
    return {};
}

std::expected<void, renaru::Error> renaru::Queue::present() const{
    if (sQueue == nullptr || sOwner == nullptr || sOwner->sDevice == nullptr){
        return std::unexpected(renaru::Error::eFramePresentationFailed);
    }

    const VkSwapchainKHR sSwapchain = sOwner->sSwapchain;
    const std::uint32_t sImageIndex = sOwner->sCurrentImage;
    const VkSemaphore sWaitSemaphore = sOwner->sRenderFinishedSemaphore();
    if (sSwapchain == nullptr || sWaitSemaphore == nullptr){
        return std::unexpected(renaru::Error::eFramePresentationFailed);
    }

    VkPresentInfoKHR sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    sInfo.waitSemaphoreCount = 1;
    sInfo.pWaitSemaphores = &sWaitSemaphore;
    sInfo.swapchainCount = 1;
    sInfo.pSwapchains = &sSwapchain;
    sInfo.pImageIndices = &sImageIndex;

    PFN_vkQueuePresentKHR pfnQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(
        entry().resolveDevice(sDevice, "vkQueuePresentKHR"));
    if (pfnQueuePresentKHR == nullptr){
        sOwner->sNoteError(renaru::Error::eFramePresentationFailed);
        return std::unexpected(renaru::Error::eFramePresentationFailed);
    }
    const VkResult sResult = pfnQueuePresentKHR(sQueue, &sInfo);
    if (sResult != VK_SUCCESS && sResult != VK_SUBOPTIMAL_KHR){
        sOwner->sNoteError(renaru::Error::eFramePresentationFailed);
        return std::unexpected(renaru::Error::eFramePresentationFailed);
    }
    return {};
}
