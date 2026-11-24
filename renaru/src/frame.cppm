module;

#include <renaru/vulkan.h>

#include <cstdint>
#include <expected>
#include <limits>

export module renaru:frame;

import :entry;
import :device;

// State frame milik Device: fence/semaphore, frame_index, image index saat ini,
// serta register swapchain/framebuffer yang dipakai closure render.
// Focus di sini hanya plumbing Vulkan C API; pemanggilannya ada di
// acquire/record/render/submit/present sesuai prototipe di main.cpp.

std::expected<void, renaru::Error> renaru::Device::ensureFrameResources() const{
    if (sdevice == nullptr || sswapchain == nullptr){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }
    if (simageAvailableSemaphores.size() == MaxFramesInFlight &&
        srenderFinishedSemaphores.size() == MaxFramesInFlight &&
        sinFlightFences.size() == MaxFramesInFlight){
        return {};
    }

    destroyFrameResources();

    PFN_vkCreateSemaphore pfnCreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(
        entry().resolveDevice(sdevice, "vkCreateSemaphore"));
    PFN_vkCreateFence pfnCreateFence = reinterpret_cast<PFN_vkCreateFence>(
        entry().resolveDevice(sdevice, "vkCreateFence"));
    if (pfnCreateSemaphore == nullptr || pfnCreateFence == nullptr){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    for (std::uint32_t sindex = 0; sindex < MaxFramesInFlight; ++sindex){
        VkSemaphoreCreateInfo ssemaphoreInfo{};
        ssemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore simageSemaphore = nullptr;
        VkSemaphore srenderSemaphore = nullptr;
        if (pfnCreateSemaphore(sdevice, &ssemaphoreInfo, nullptr, &simageSemaphore) != VK_SUCCESS ||
            pfnCreateSemaphore(sdevice, &ssemaphoreInfo, nullptr, &srenderSemaphore) != VK_SUCCESS){
            if (simageSemaphore != nullptr){
                PFN_vkDestroySemaphore pfnDestroySemaphore =
                    reinterpret_cast<PFN_vkDestroySemaphore>(
                        entry().resolveDevice(sdevice, "vkDestroySemaphore"));
                if (pfnDestroySemaphore != nullptr){
                    pfnDestroySemaphore(sdevice, simageSemaphore, nullptr);
                }
            }
            if (srenderSemaphore != nullptr){
                PFN_vkDestroySemaphore pfnDestroySemaphore =
                    reinterpret_cast<PFN_vkDestroySemaphore>(
                        entry().resolveDevice(sdevice, "vkDestroySemaphore"));
                if (pfnDestroySemaphore != nullptr){
                    pfnDestroySemaphore(sdevice, srenderSemaphore, nullptr);
                }
            }
            destroyFrameResources();
            return std::unexpected(renaru::Error::FrameAcquisitionFailed);
        }

        VkFenceCreateInfo sfenceInfo{};
        sfenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        sfenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkFence sfence = nullptr;
        if (pfnCreateFence(sdevice, &sfenceInfo, nullptr, &sfence) != VK_SUCCESS){
            PFN_vkDestroySemaphore pfnDestroySemaphore =
                reinterpret_cast<PFN_vkDestroySemaphore>(
                    entry().resolveDevice(sdevice, "vkDestroySemaphore"));
            if (pfnDestroySemaphore != nullptr){
                pfnDestroySemaphore(sdevice, simageSemaphore, nullptr);
                pfnDestroySemaphore(sdevice, srenderSemaphore, nullptr);
            }
            destroyFrameResources();
            return std::unexpected(renaru::Error::FrameAcquisitionFailed);
        }

        simageAvailableSemaphores.push_back(simageSemaphore);
        srenderFinishedSemaphores.push_back(srenderSemaphore);
        sinFlightFences.push_back(sfence);
    }

    if (simageAvailableSemaphores.size() != MaxFramesInFlight ||
        srenderFinishedSemaphores.size() != MaxFramesInFlight ||
        sinFlightFences.size() != MaxFramesInFlight){
        destroyFrameResources();
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    return {};
}

void renaru::Device::destroyFrameResources() const noexcept{
    if (sdevice == nullptr){
        simageAvailableSemaphores.clear();
        srenderFinishedSemaphores.clear();
        sinFlightFences.clear();
        return;
    }

    PFN_vkDestroySemaphore pfnDestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(
        entry().resolveDevice(sdevice, "vkDestroySemaphore"));
    PFN_vkDestroyFence pfnDestroyFence = reinterpret_cast<PFN_vkDestroyFence>(
        entry().resolveDevice(sdevice, "vkDestroyFence"));

    if (pfnDestroySemaphore != nullptr){
        for (const VkSemaphore ssemaphore : simageAvailableSemaphores){
            if (ssemaphore != nullptr){
                pfnDestroySemaphore(sdevice, ssemaphore, nullptr);
            }
        }
        for (const VkSemaphore ssemaphore : srenderFinishedSemaphores){
            if (ssemaphore != nullptr){
                pfnDestroySemaphore(sdevice, ssemaphore, nullptr);
            }
        }
    }
    if (pfnDestroyFence != nullptr){
        for (const VkFence sfence : sinFlightFences){
            if (sfence != nullptr){
                pfnDestroyFence(sdevice, sfence, nullptr);
            }
        }
    }

    simageAvailableSemaphores.clear();
    srenderFinishedSemaphores.clear();
    sinFlightFences.clear();
}

std::expected<void, renaru::Error> renaru::Device::beginFrame() const{
    if (auto ready = ensureFrameResources(); !ready){
        return ready;
    }
    if (sframeIndex >= sinFlightFences.size()){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    slastError = renaru::Error::None;
    sactiveCommandBuffer = nullptr;

    const VkFence sfence = sinFlightFences[sframeIndex];
    const std::uint64_t stimeout = (std::numeric_limits<std::uint64_t>::max)();

    PFN_vkWaitForFences pfnWaitForFences = reinterpret_cast<PFN_vkWaitForFences>(
        entry().resolveDevice(sdevice, "vkWaitForFences"));
    PFN_vkResetFences pfnResetFences = reinterpret_cast<PFN_vkResetFences>(
        entry().resolveDevice(sdevice, "vkResetFences"));
    PFN_vkAcquireNextImageKHR pfnAcquireNextImageKHR =
        reinterpret_cast<PFN_vkAcquireNextImageKHR>(
            entry().resolveDevice(sdevice, "vkAcquireNextImageKHR"));
    if (pfnWaitForFences == nullptr || pfnResetFences == nullptr || pfnAcquireNextImageKHR == nullptr){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    if (pfnWaitForFences(sdevice, 1, &sfence, VK_TRUE, stimeout) != VK_SUCCESS){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }
    if (pfnResetFences(sdevice, 1, &sfence) != VK_SUCCESS){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    const VkResult sacquired = pfnAcquireNextImageKHR(
        sdevice, sswapchain, stimeout,
        simageAvailableSemaphores[sframeIndex], nullptr, &scurrentImage);
    if (sacquired != VK_SUCCESS && sacquired != VK_SUBOPTIMAL_KHR){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    return {};
}

void renaru::Device::waitIdle() const noexcept{
    if (sdevice == nullptr){
        return;
    }
    PFN_vkDeviceWaitIdle pfnDeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(
        entry().resolveDevice(sdevice, "vkDeviceWaitIdle"));
    if (pfnDeviceWaitIdle != nullptr){
        pfnDeviceWaitIdle(sdevice);
    }
}

void renaru::Device::finishFrame() const noexcept{
    if (!sinFlightFences.empty()){
        sframeIndex = (sframeIndex + 1) %
                            static_cast<std::uint32_t>(sinFlightFences.size());
    }
    sactiveCommandBuffer = nullptr;
}

void renaru::Device::registerSwapchain(VkSwapchainKHR snewSwapchain,
                                       VkExtent2D snewExtent,
                                       std::uint32_t snewImageCount) const noexcept{
    sswapchain = snewSwapchain;
    sframeExtent = snewExtent;
    sframebuffers.clear();
    sframebuffers.reserve(snewImageCount);
    sframeIndex = 0;
    scurrentImage = 0;
    destroyFrameResources();
}

void renaru::Device::registerFramebuffer(VkFramebuffer sframebuffer) const noexcept{
    if (sframebuffer != nullptr){
        sframebuffers.push_back(sframebuffer);
    }
}

VkSwapchainKHR renaru::Device::swapchainHandle() const noexcept{
    return sswapchain;
}

VkFramebuffer renaru::Device::framebufferForCurrentImage() const noexcept{
    if (scurrentImage >= sframebuffers.size()){
        return nullptr;
    }
    return sframebuffers[scurrentImage];
}

VkExtent2D renaru::Device::frameExtent() const noexcept{
    return sframeExtent;
}

VkSemaphore renaru::Device::imageAvailableSemaphore() const noexcept{
    if (sframeIndex >= simageAvailableSemaphores.size()){
        return nullptr;
    }
    return simageAvailableSemaphores[sframeIndex];
}

VkSemaphore renaru::Device::renderFinishedSemaphore() const noexcept{
    if (sframeIndex >= srenderFinishedSemaphores.size()){
        return nullptr;
    }
    return srenderFinishedSemaphores[sframeIndex];
}

VkFence renaru::Device::currentFence() const noexcept{
    if (sframeIndex >= sinFlightFences.size()){
        return nullptr;
    }
    return sinFlightFences[sframeIndex];
}

std::uint32_t renaru::Device::currentImageIndex() const noexcept{
    return scurrentImage;
}

std::uint32_t renaru::Device::frameIndex() const noexcept{
    return sframeIndex;
}

VkCommandBuffer renaru::Device::activeCommandBuffer() const noexcept{
    return sactiveCommandBuffer;
}

void renaru::Device::setActiveCommandBuffer(VkCommandBuffer sbuffer) const noexcept{
    sactiveCommandBuffer = sbuffer;
}

void renaru::Device::clearActiveCommandBuffer() const noexcept{
    sactiveCommandBuffer = nullptr;
}

void renaru::Device::noteError(renaru::Error serror) const noexcept{
    if (slastError == renaru::Error::None){
        slastError = serror;
    }
}

renaru::Error renaru::Device::lastError() const noexcept{
    return slastError;
}
