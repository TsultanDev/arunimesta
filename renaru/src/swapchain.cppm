module;

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <expected>
#include <limits>
#include <print>
#include <utility>
#include <vector>

export module renaru:swapchain;

import :entry;
import :device;
import :resource;

export namespace renaru {
    class Swapchain {
        private:
            using ResultType = std::expected<Swapchain, Error>;

            VkDevice sDevice = nullptr;
            VkSwapchainKHR sSwapchain = nullptr;
            VkFormat sFormat = VK_FORMAT_UNDEFINED;
            VkExtent2D sExtent{};
            std::uint32_t sImageCount = 0;

            Swapchain(VkDevice sDevice, VkSwapchainKHR sSwapchain,
                      VkFormat sFormat, VkExtent2D sExtent,
                      std::uint32_t sImageCount) noexcept :
                sDevice(sDevice),
                sSwapchain(sSwapchain),
                sFormat(sFormat),
                sExtent(sExtent),
                sImageCount(sImageCount)
            {}

            void sDestroy() noexcept{
                if (sSwapchain != nullptr && sDevice != nullptr){
                    PFN_vkDestroySwapchainKHR pfnDestroySwapchainKHR =
                        reinterpret_cast<PFN_vkDestroySwapchainKHR>(
                            entry().resolveDevice(sDevice, "vkDestroySwapchainKHR"));
                    if (pfnDestroySwapchainKHR != nullptr){
                        pfnDestroySwapchainKHR(sDevice, sSwapchain, nullptr);
                    }
                    sSwapchain = nullptr;
                }
                sDevice = nullptr;
                sExtent = VkExtent2D{};
                sImageCount = 0;
            }

            friend class Device;

        public:
            ~Swapchain(){ sDestroy(); }

            Swapchain(const Swapchain&) = delete;
            Swapchain& operator=(const Swapchain&) = delete;

            Swapchain(Swapchain&& sOther) noexcept :
                sDevice(std::exchange(sOther.sDevice, nullptr)),
                sSwapchain(std::exchange(sOther.sSwapchain, nullptr)),
                sFormat(sOther.sFormat),
                sExtent(std::exchange(sOther.sExtent, VkExtent2D{})),
                sImageCount(std::exchange(sOther.sImageCount, std::uint32_t{0}))
            {}

            Swapchain& operator=(Swapchain&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sDevice = std::exchange(sOther.sDevice, nullptr);
                    sSwapchain = std::exchange(sOther.sSwapchain, nullptr);
                    sFormat = sOther.sFormat;
                    sExtent = std::exchange(sOther.sExtent, VkExtent2D{});
                    sImageCount = std::exchange(sOther.sImageCount, std::uint32_t{0});
                }
                return *this;
            }
    };
}

std::expected<renaru::Swapchain, renaru::Error>
renaru::Device::createSwapchain(std::uint32_t sWidth, std::uint32_t sHeight) const{
    if (sDevice == nullptr || sSurface == nullptr){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }

    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfnGetCapabilities =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            entry().resolveInstance(sInstance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pfnGetFormats =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            entry().resolveInstance(sInstance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pfnGetPresentModes =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
            entry().resolveInstance(sInstance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    if (pfnGetCapabilities == nullptr || pfnGetFormats == nullptr || pfnGetPresentModes == nullptr){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }

    VkSurfaceCapabilitiesKHR sCapabilities{};
    if (pfnGetCapabilities(sPhysicalDevice, sSurface, &sCapabilities) != VK_SUCCESS){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }

    std::uint32_t sFormatCount = 0;
    if (pfnGetFormats(sPhysicalDevice, sSurface, &sFormatCount, nullptr) != VK_SUCCESS ||
        sFormatCount == 0){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }
    std::vector<VkSurfaceFormatKHR> sFormats(sFormatCount);
    if (pfnGetFormats(sPhysicalDevice, sSurface, &sFormatCount, sFormats.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }

    std::uint32_t sModeCount = 0;
    if (pfnGetPresentModes(sPhysicalDevice, sSurface, &sModeCount, nullptr) != VK_SUCCESS ||
        sModeCount == 0){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }
    std::vector<VkPresentModeKHR> sModes(sModeCount);
    if (pfnGetPresentModes(sPhysicalDevice, sSurface, &sModeCount, sModes.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }

    VkSurfaceFormatKHR sChosenFormat = sFormats.front();
    for (const VkSurfaceFormatKHR& sFormat : sFormats){
        if (sFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            sFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            sChosenFormat = sFormat;
            break;
        }
    }

    VkPresentModeKHR sChosenMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const VkPresentModeKHR sMode : sModes){
        if (sMode == VK_PRESENT_MODE_MAILBOX_KHR){
            sChosenMode = sMode;
            break;
        }
    }

    VkExtent2D sExtent{};
    const std::uint32_t sUndefinedExtent = (std::numeric_limits<std::uint32_t>::max)();
    if (sCapabilities.currentExtent.width != sUndefinedExtent &&
        sCapabilities.currentExtent.height != sUndefinedExtent){
        sExtent = sCapabilities.currentExtent;
    }else{
        sExtent.width = std::clamp(sWidth, sCapabilities.minImageExtent.width,
                                   sCapabilities.maxImageExtent.width);
        sExtent.height = std::clamp(sHeight, sCapabilities.minImageExtent.height,
                                    sCapabilities.maxImageExtent.height);
    }

    std::uint32_t sImageCount = sCapabilities.minImageCount + 1;
    if (sCapabilities.maxImageCount > 0 && sImageCount > sCapabilities.maxImageCount){
        sImageCount = sCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sInfo.surface = sSurface;
    sInfo.minImageCount = sImageCount;
    sInfo.imageFormat = sChosenFormat.format;
    sInfo.imageColorSpace = sChosenFormat.colorSpace;
    sInfo.imageExtent = sExtent;
    sInfo.imageArrayLayers = 1;
    sInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sInfo.preTransform = sCapabilities.currentTransform;
    sInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sInfo.presentMode = sChosenMode;
    sInfo.clipped = VK_TRUE;
    sInfo.oldSwapchain = nullptr;

    PFN_vkCreateSwapchainKHR pfnCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(
        entry().resolveDevice(sDevice, "vkCreateSwapchainKHR"));
    if (pfnCreateSwapchainKHR == nullptr){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }

    VkSwapchainKHR sCreated = nullptr;
    if (pfnCreateSwapchainKHR(sDevice, &sInfo, nullptr, &sCreated) != VK_SUCCESS ||
        sCreated == nullptr){
        return std::unexpected(renaru::Error::eSwapchainCreationFailed);
    }

    std::println("Aru RHI: Swapchain(created!!) images={}", sImageCount);

    sRegisterSwapchain(sCreated, sExtent, sImageCount);

    return Swapchain(sDevice, sCreated, sChosenFormat.format, sExtent, sImageCount);
}

std::expected<std::vector<renaru::Resource>, renaru::Error>
renaru::Device::createSwapchainImages(const renaru::Swapchain& sSwapchain) const{
    if (sDevice == nullptr || sSwapchain.sSwapchain == nullptr){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    PFN_vkGetSwapchainImagesKHR pfnGetSwapchainImagesKHR =
        reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(
            entry().resolveDevice(sDevice, "vkGetSwapchainImagesKHR"));
    PFN_vkCreateImageView pfnCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(
        entry().resolveDevice(sDevice, "vkCreateImageView"));
    if (pfnGetSwapchainImagesKHR == nullptr || pfnCreateImageView == nullptr){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    std::uint32_t sCount = 0;
    if (pfnGetSwapchainImagesKHR(sDevice, sSwapchain.sSwapchain, &sCount, nullptr) != VK_SUCCESS ||
        sCount == 0){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }
    std::vector<VkImage> sImages(sCount);
    if (pfnGetSwapchainImagesKHR(sDevice, sSwapchain.sSwapchain, &sCount, sImages.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    std::vector<renaru::Resource> sResources;
    sResources.reserve(sImages.size());

    for (const VkImage sImage : sImages){
        VkImageViewCreateInfo sInfo{};
        sInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        sInfo.image = sImage;
        sInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        sInfo.format = sSwapchain.sFormat;
        sInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        sInfo.subresourceRange.baseMipLevel = 0;
        sInfo.subresourceRange.levelCount = 1;
        sInfo.subresourceRange.baseArrayLayer = 0;
        sInfo.subresourceRange.layerCount = 1;

        VkImageView sView = nullptr;
        if (pfnCreateImageView(sDevice, &sInfo, nullptr, &sView) != VK_SUCCESS ||
            sView == nullptr){
            return std::unexpected(renaru::Error::eResourceCreationFailed);
        }

        sResources.push_back(renaru::Resource(
            sDevice, sImage, sView, sSwapchain.sExtent
        ));
    }

    std::println("Aru RHI: Swapchain images(image views ready) count={}", sImages.size());

    return sResources;
}
