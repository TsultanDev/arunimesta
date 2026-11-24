module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <limits>

export module renaru:frame;

import :entry;
import :device;

// State frame milik Device: fence/semaphore, frame_index, image index saat ini,
// serta register swapchain/framebuffer yang dipakai closure render.

std::expected<void, renaru::Error> renaru::Device::sEnsureFrameResources() const{
    if (sDevice == nullptr || sSwapchain == nullptr){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }
    if (sImageAvailableSemaphores.size() == MaxFramesInFlight &&
        sRenderFinishedSemaphores.size() == MaxFramesInFlight &&
        sInFlightFences.size() == MaxFramesInFlight){
        return {};
    }

    sDestroyFrameResources();

    PFN_vkCreateSemaphore pfnCreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(
        entry().resolveDevice(sDevice, "vkCreateSemaphore"));
    PFN_vkCreateFence pfnCreateFence = reinterpret_cast<PFN_vkCreateFence>(
        entry().resolveDevice(sDevice, "vkCreateFence"));
    if (pfnCreateSemaphore == nullptr || pfnCreateFence == nullptr){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    for (std::uint32_t sIndex = 0; sIndex < MaxFramesInFlight; ++sIndex){
        VkSemaphoreCreateInfo sSemaphoreInfo{};
        sSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore sImageSemaphore = nullptr;
        VkSemaphore sRenderSemaphore = nullptr;
        if (pfnCreateSemaphore(sDevice, &sSemaphoreInfo, nullptr, &sImageSemaphore) != VK_SUCCESS ||
            pfnCreateSemaphore(sDevice, &sSemaphoreInfo, nullptr, &sRenderSemaphore) != VK_SUCCESS){
            if (sImageSemaphore != nullptr){
                PFN_vkDestroySemaphore pfnDestroySemaphore =
                    reinterpret_cast<PFN_vkDestroySemaphore>(
                        entry().resolveDevice(sDevice, "vkDestroySemaphore"));
                if (pfnDestroySemaphore != nullptr){
                    pfnDestroySemaphore(sDevice, sImageSemaphore, nullptr);
                }
            }
            if (sRenderSemaphore != nullptr){
                PFN_vkDestroySemaphore pfnDestroySemaphore =
                    reinterpret_cast<PFN_vkDestroySemaphore>(
                        entry().resolveDevice(sDevice, "vkDestroySemaphore"));
                if (pfnDestroySemaphore != nullptr){
                    pfnDestroySemaphore(sDevice, sRenderSemaphore, nullptr);
                }
            }
            sDestroyFrameResources();
            return std::unexpected(renaru::Error::FrameAcquisitionFailed);
        }

        VkFenceCreateInfo sFenceInfo{};
        sFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        sFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkFence sFence = nullptr;
        if (pfnCreateFence(sDevice, &sFenceInfo, nullptr, &sFence) != VK_SUCCESS){
            PFN_vkDestroySemaphore pfnDestroySemaphore =
                reinterpret_cast<PFN_vkDestroySemaphore>(
                    entry().resolveDevice(sDevice, "vkDestroySemaphore"));
            if (pfnDestroySemaphore != nullptr){
                pfnDestroySemaphore(sDevice, sImageSemaphore, nullptr);
                pfnDestroySemaphore(sDevice, sRenderSemaphore, nullptr);
            }
            sDestroyFrameResources();
            return std::unexpected(renaru::Error::FrameAcquisitionFailed);
        }

        sImageAvailableSemaphores.push_back(sImageSemaphore);
        sRenderFinishedSemaphores.push_back(sRenderSemaphore);
        sInFlightFences.push_back(sFence);
    }

    if (sImageAvailableSemaphores.size() != MaxFramesInFlight ||
        sRenderFinishedSemaphores.size() != MaxFramesInFlight ||
        sInFlightFences.size() != MaxFramesInFlight){
        sDestroyFrameResources();
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    return {};
}

void renaru::Device::sDestroyFrameResources() const noexcept{
    if (sDevice == nullptr){
        sImageAvailableSemaphores.clear();
        sRenderFinishedSemaphores.clear();
        sInFlightFences.clear();
        return;
    }

    PFN_vkDestroySemaphore pfnDestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(
        entry().resolveDevice(sDevice, "vkDestroySemaphore"));
    PFN_vkDestroyFence pfnDestroyFence = reinterpret_cast<PFN_vkDestroyFence>(
        entry().resolveDevice(sDevice, "vkDestroyFence"));

    if (pfnDestroySemaphore != nullptr){
        for (const VkSemaphore sSemaphore : sImageAvailableSemaphores){
            if (sSemaphore != nullptr){
                pfnDestroySemaphore(sDevice, sSemaphore, nullptr);
            }
        }
        for (const VkSemaphore sSemaphore : sRenderFinishedSemaphores){
            if (sSemaphore != nullptr){
                pfnDestroySemaphore(sDevice, sSemaphore, nullptr);
            }
        }
    }
    if (pfnDestroyFence != nullptr){
        for (const VkFence sFence : sInFlightFences){
            if (sFence != nullptr){
                pfnDestroyFence(sDevice, sFence, nullptr);
            }
        }
    }

    sImageAvailableSemaphores.clear();
    sRenderFinishedSemaphores.clear();
    sInFlightFences.clear();
}

std::expected<void, renaru::Error> renaru::Device::sBeginFrame() const{
    if (auto ready = sEnsureFrameResources(); !ready){
        return ready;
    }
    if (sFrameIndex >= sInFlightFences.size()){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    sLastError = renaru::Error::None;
    sActiveCommandBuffer = nullptr;

    const VkFence sFence = sInFlightFences[sFrameIndex];
    const std::uint64_t sTimeout = (std::numeric_limits<std::uint64_t>::max)();

    PFN_vkWaitForFences pfnWaitForFences = reinterpret_cast<PFN_vkWaitForFences>(
        entry().resolveDevice(sDevice, "vkWaitForFences"));
    PFN_vkResetFences pfnResetFences = reinterpret_cast<PFN_vkResetFences>(
        entry().resolveDevice(sDevice, "vkResetFences"));
    PFN_vkAcquireNextImageKHR pfnAcquireNextImageKHR =
        reinterpret_cast<PFN_vkAcquireNextImageKHR>(
            entry().resolveDevice(sDevice, "vkAcquireNextImageKHR"));
    if (pfnWaitForFences == nullptr || pfnResetFences == nullptr || pfnAcquireNextImageKHR == nullptr){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    if (pfnWaitForFences(sDevice, 1, &sFence, VK_TRUE, sTimeout) != VK_SUCCESS){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }
    if (pfnResetFences(sDevice, 1, &sFence) != VK_SUCCESS){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    const VkResult sAcquired = pfnAcquireNextImageKHR(
        sDevice, sSwapchain, sTimeout,
        sImageAvailableSemaphores[sFrameIndex], nullptr, &sCurrentImage);
    if (sAcquired != VK_SUCCESS && sAcquired != VK_SUBOPTIMAL_KHR){
        return std::unexpected(renaru::Error::FrameAcquisitionFailed);
    }

    return {};
}

void renaru::Device::waitIdle() const noexcept{
    if (sDevice == nullptr){
        return;
    }
    PFN_vkDeviceWaitIdle pfnDeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(
        entry().resolveDevice(sDevice, "vkDeviceWaitIdle"));
    if (pfnDeviceWaitIdle != nullptr){
        pfnDeviceWaitIdle(sDevice);
    }
}

void renaru::Device::sFinishFrame() const noexcept{
    if (!sInFlightFences.empty()){
        sFrameIndex = (sFrameIndex + 1) %
                      static_cast<std::uint32_t>(sInFlightFences.size());
    }
    sActiveCommandBuffer = nullptr;
}

void renaru::Device::sRegisterSwapchain(VkSwapchainKHR sNewSwapchain,
                                        VkExtent2D sNewExtent,
                                        std::uint32_t sNewImageCount) const noexcept{
    sSwapchain = sNewSwapchain;
    sFrameExtent = sNewExtent;
    sFramebuffers.clear();
    sFramebuffers.reserve(sNewImageCount);
    sFrameIndex = 0;
    sCurrentImage = 0;
    sDestroyFrameResources();
}

void renaru::Device::sRegisterFramebuffer(VkFramebuffer sNewFramebuffer) const noexcept{
    if (sNewFramebuffer != nullptr){
        sFramebuffers.push_back(sNewFramebuffer);
    }
}

VkFramebuffer renaru::Device::sFramebufferForCurrentImage() const noexcept{
    if (sCurrentImage >= sFramebuffers.size()){
        return nullptr;
    }
    return sFramebuffers[sCurrentImage];
}

VkExtent2D renaru::Device::sCurrentFrameExtent() const noexcept{
    return sFrameExtent;
}

VkSemaphore renaru::Device::sImageAvailableSemaphore() const noexcept{
    if (sFrameIndex >= sImageAvailableSemaphores.size()){
        return nullptr;
    }
    return sImageAvailableSemaphores[sFrameIndex];
}

VkSemaphore renaru::Device::sRenderFinishedSemaphore() const noexcept{
    if (sFrameIndex >= sRenderFinishedSemaphores.size()){
        return nullptr;
    }
    return sRenderFinishedSemaphores[sFrameIndex];
}

VkFence renaru::Device::sCurrentFence() const noexcept{
    if (sFrameIndex >= sInFlightFences.size()){
        return nullptr;
    }
    return sInFlightFences[sFrameIndex];
}

void renaru::Device::sNoteError(renaru::Error sError) const noexcept{
    if (sLastError == renaru::Error::None){
        sLastError = sError;
    }
}

renaru::Error renaru::Device::sCurrentError() const noexcept{
    return sLastError;
}
