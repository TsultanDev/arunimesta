module;

#include <renaru/vulkan.h>

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
        public:
            using ResultType = std::expected<CommandBuffer, Error>;

            // VkCommandBuffer adalah raw handle: salinan tidak diizinkan.
            CommandBuffer(const CommandBuffer&) = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;

            CommandBuffer(CommandBuffer&& sother) noexcept :
                sowner(std::exchange(sother.sowner, nullptr)),
                sdevice(std::exchange(sother.sdevice, nullptr)),
                spool(std::exchange(sother.spool, nullptr)),
                sbuffer(std::exchange(sother.sbuffer, nullptr))
            {}

            CommandBuffer& operator=(CommandBuffer&& sother) noexcept{
                if (this != &sother){
                    release();
                    sowner = std::exchange(sother.sowner, nullptr);
                    sdevice = std::exchange(sother.sdevice, nullptr);
                    spool = std::exchange(sother.spool, nullptr);
                    sbuffer = std::exchange(sother.sbuffer, nullptr);
                }
                return *this;
            }

            ~CommandBuffer(){ release(); }

            template<typename Func>
            std::expected<void, Error> record(Func&& sfunc){
                if (auto started = beginRecording(); !started){
                    return std::unexpected(started.error());
                }

                try{
                    std::forward<Func>(sfunc)();
                }catch(...){
                    endRecording();
                    const renaru::Error serror = renaru::Error::CommandBufferRecordingFailed;
                    if (sowner != nullptr){
                        sowner->noteError(serror);
                    }
                    return std::unexpected(serror);
                }

                endRecording();
                if (sowner != nullptr && sowner->lastError() != Error::None){
                    return std::unexpected(sowner->lastError());
                }
                return {};
            }

            VkCommandBuffer handle() const noexcept { return sbuffer; }

            VkDevice deviceHandle() const noexcept { return sdevice; }

            VkCommandPool poolHandle() const noexcept { return spool; }

            bool empty() const noexcept { return sbuffer == nullptr; }

            bool begin(VkCommandBufferUsageFlags sflags = 0) noexcept{
                if (sbuffer == nullptr || sdevice == nullptr){
                    return false;
                }
                PFN_vkBeginCommandBuffer pfnBeginCommandBuffer =
                    reinterpret_cast<PFN_vkBeginCommandBuffer>(
                        entry().resolveDevice(sdevice, "vkBeginCommandBuffer"));
                if (pfnBeginCommandBuffer == nullptr){
                    return false;
                }

                VkCommandBufferBeginInfo sinfo{};
                sinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                sinfo.flags = sflags;
                return pfnBeginCommandBuffer(sbuffer, &sinfo) == VK_SUCCESS;
            }

            void end() noexcept{
                if (sbuffer == nullptr || sdevice == nullptr){
                    return;
                }
                PFN_vkEndCommandBuffer pfnEndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(
                    entry().resolveDevice(sdevice, "vkEndCommandBuffer"));
                if (pfnEndCommandBuffer != nullptr){
                    pfnEndCommandBuffer(sbuffer);
                }
            }

            bool reset(VkCommandBufferResetFlags sflags = 0) noexcept{
                if (sbuffer == nullptr || sdevice == nullptr){
                    return false;
                }
                PFN_vkResetCommandBuffer pfnResetCommandBuffer =
                    reinterpret_cast<PFN_vkResetCommandBuffer>(
                        entry().resolveDevice(sdevice, "vkResetCommandBuffer"));
                if (pfnResetCommandBuffer == nullptr){
                    return false;
                }
                return pfnResetCommandBuffer(sbuffer, sflags) == VK_SUCCESS;
            }

        private:
            friend class Device;

            CommandBuffer(const Device* sowner, VkDevice sdevice, VkCommandPool spool,
                          VkCommandBuffer sbuffer) noexcept :
                sowner(sowner),
                sdevice(sdevice),
                spool(spool),
                sbuffer(sbuffer)
            {}

            std::expected<void, Error> beginRecording(){
                if (sbuffer == nullptr || sowner == nullptr || sowner->empty()){
                    return std::unexpected(renaru::Error::CommandBufferRecordingFailed);
                }

                // Pool dibuat dengan RESET_COMMAND_BUFFER, jadi reset diperbolehkan.
                // Di state awal reset boleh gagal; begin() tetap bisa dipanggil.
                (void)reset();
                if (!begin()){
                    return std::unexpected(renaru::Error::CommandBufferRecordingFailed);
                }

                sowner->setActiveCommandBuffer(sbuffer);
                return {};
            }

            void endRecording() noexcept{
                if (sowner != nullptr){
                    sowner->clearActiveCommandBuffer();
                }
                end();
            }

            void release() noexcept{
                if (sbuffer != nullptr && sdevice != nullptr && spool != nullptr){
                    PFN_vkFreeCommandBuffers pfnFreeCommandBuffers =
                        reinterpret_cast<PFN_vkFreeCommandBuffers>(
                            entry().resolveDevice(sdevice, "vkFreeCommandBuffers"));
                    if (pfnFreeCommandBuffers != nullptr){
                        pfnFreeCommandBuffers(sdevice, spool, 1, &sbuffer);
                    }
                    sbuffer = nullptr;
                }
                sowner = nullptr;
                sdevice = nullptr;
                spool = nullptr;
            }

            const Device* sowner = nullptr;
            VkDevice sdevice = nullptr;
            VkCommandPool spool = nullptr;
            VkCommandBuffer sbuffer = nullptr;
    };
}

// Definisi lintas partisi: Device/Queue didefinisikan di partisi sebelumnya,
// tetapi alokasi dan submit membutuhkan tipe lengkap CommandBuffer di sini.
std::expected<std::vector<renaru::CommandBuffer>, renaru::Error>
renaru::Device::allocateCommandBuffers(const renaru::CommandPool& scommandPool,
                                       std::uint32_t scount) const{
    if (sdevice == nullptr || scommandPool.empty() || scount == 0){
        return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
    }
    if (scommandPool.deviceHandle() != sdevice){
        return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
    }

    PFN_vkAllocateCommandBuffers pfnAllocateCommandBuffers =
        reinterpret_cast<PFN_vkAllocateCommandBuffers>(
            entry().resolveDevice(sdevice, "vkAllocateCommandBuffers"));
    PFN_vkFreeCommandBuffers pfnFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(
        entry().resolveDevice(sdevice, "vkFreeCommandBuffers"));
    if (pfnAllocateCommandBuffers == nullptr || pfnFreeCommandBuffers == nullptr){
        return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
    }

    VkCommandBufferAllocateInfo sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    sinfo.commandPool = scommandPool.handle();
    sinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    sinfo.commandBufferCount = scount;

    std::vector<VkCommandBuffer> sallocated(scount, nullptr);
    if (pfnAllocateCommandBuffers(sdevice, &sinfo, sallocated.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
    }

    for (const VkCommandBuffer sbuffer : sallocated){
        if (sbuffer == nullptr){
            for (const VkCommandBuffer screated : sallocated){
                if (screated != nullptr){
                    pfnFreeCommandBuffers(sdevice, scommandPool.handle(), 1, &screated);
                }
            }
            return std::unexpected(renaru::Error::CommandBufferAllocationFailed);
        }
    }

    std::vector<CommandBuffer> sresult;
    sresult.reserve(sallocated.size());
    for (const VkCommandBuffer sbuffer : sallocated){
        sresult.push_back(CommandBuffer(this, sdevice, scommandPool.handle(), sbuffer));
    }

    std::println("Aru RHI: CommandBuffers(allocated!!) count={}", sresult.size());

    return sresult;
}

std::expected<void, renaru::Error> renaru::Queue::submit(
    const renaru::CommandBuffer& sbuffer) const{
    if (squeue == nullptr || sowner == nullptr || sowner->empty() || sbuffer.empty()){
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
    }
    if (sbuffer.deviceHandle() != sowner->handle()){
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
    }

    const VkCommandBuffer scommandBuffer = sbuffer.handle();
    const VkSemaphore swaitSemaphore = sowner->imageAvailableSemaphore();
    const VkSemaphore ssignalSemaphore = sowner->renderFinishedSemaphore();
    const VkFence sfence = sowner->currentFence();
    if (swaitSemaphore == nullptr || ssignalSemaphore == nullptr || sfence == nullptr){
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
    }

    const VkPipelineStageFlags swaitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    sinfo.waitSemaphoreCount = 1;
    sinfo.pWaitSemaphores = &swaitSemaphore;
    sinfo.pWaitDstStageMask = swaitStages;
    sinfo.commandBufferCount = 1;
    sinfo.pCommandBuffers = &scommandBuffer;
    sinfo.signalSemaphoreCount = 1;
    sinfo.pSignalSemaphores = &ssignalSemaphore;

    PFN_vkQueueSubmit pfnQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(
        entry().resolveDevice(sdevice, "vkQueueSubmit"));
    if (pfnQueueSubmit == nullptr){
        sowner->noteError(renaru::Error::CommandSubmissionFailed);
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
    }
    if (pfnQueueSubmit(squeue, 1, &sinfo, sfence) != VK_SUCCESS){
        sowner->noteError(renaru::Error::CommandSubmissionFailed);
        return std::unexpected(renaru::Error::CommandSubmissionFailed);
    }
    return {};
}

std::expected<void, renaru::Error> renaru::Queue::present() const{
    if (squeue == nullptr || sowner == nullptr || sowner->empty()){
        return std::unexpected(renaru::Error::FramePresentationFailed);
    }

    const VkSwapchainKHR sswapchain = sowner->swapchainHandle();
    const std::uint32_t simageIndex = sowner->currentImageIndex();
    const VkSemaphore swaitSemaphore = sowner->renderFinishedSemaphore();
    if (sswapchain == nullptr || swaitSemaphore == nullptr){
        return std::unexpected(renaru::Error::FramePresentationFailed);
    }

    VkPresentInfoKHR sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    sinfo.waitSemaphoreCount = 1;
    sinfo.pWaitSemaphores = &swaitSemaphore;
    sinfo.swapchainCount = 1;
    sinfo.pSwapchains = &sswapchain;
    sinfo.pImageIndices = &simageIndex;

    PFN_vkQueuePresentKHR pfnQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(
        entry().resolveDevice(sdevice, "vkQueuePresentKHR"));
    if (pfnQueuePresentKHR == nullptr){
        sowner->noteError(renaru::Error::FramePresentationFailed);
        return std::unexpected(renaru::Error::FramePresentationFailed);
    }
    const VkResult sresult = pfnQueuePresentKHR(squeue, &sinfo);
    if (sresult != VK_SUCCESS && sresult != VK_SUBOPTIMAL_KHR){
        sowner->noteError(renaru::Error::FramePresentationFailed);
        return std::unexpected(renaru::Error::FramePresentationFailed);
    }
    return {};
}
