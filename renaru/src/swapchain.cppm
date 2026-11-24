module;

#include <renaru/vulkan.h>

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
        public:
            using ResultType = std::expected<Swapchain, Error>;

            // VkSwapchainKHR adalah raw handle: salinan tidak diizinkan.
            Swapchain(const Swapchain&) = delete;
            Swapchain& operator=(const Swapchain&) = delete;

            Swapchain(Swapchain&& sother) noexcept :
                sdevice(std::exchange(sother.sdevice, nullptr)),
                sswapchain(std::exchange(sother.sswapchain, nullptr)),
                sformat(sother.sformat),
                sextent(std::exchange(sother.sextent, VkExtent2D{})),
                simageCount(std::exchange(sother.simageCount, std::uint32_t{0}))
            {}

            Swapchain& operator=(Swapchain&& sother) noexcept{
                if (this != &sother){
                    release();
                    sdevice = std::exchange(sother.sdevice, nullptr);
                    sswapchain = std::exchange(sother.sswapchain, nullptr);
                    sformat = sother.sformat;
                    sextent = std::exchange(sother.sextent, VkExtent2D{});
                    simageCount = std::exchange(sother.simageCount, std::uint32_t{0});
                }
                return *this;
            }

            ~Swapchain(){ release(); }

            VkSwapchainKHR handle() const noexcept { return sswapchain; }

            VkDevice deviceHandle() const noexcept { return sdevice; }

            VkFormat format() const noexcept { return sformat; }

            VkExtent2D extent() const noexcept { return sextent; }

            std::uint32_t imageCount() const noexcept { return simageCount; }

            bool empty() const noexcept { return sswapchain == nullptr; }

        private:
            friend class Device;

            Swapchain(VkDevice sdevice, VkSwapchainKHR sswapchain,
                      VkFormat sformat, VkExtent2D sextent,
                      std::uint32_t simageCount) noexcept :
                sdevice(sdevice),
                sswapchain(sswapchain),
                sformat(sformat),
                sextent(sextent),
                simageCount(simageCount)
            {}

            void release() noexcept{
                if (sswapchain != nullptr && sdevice != nullptr){
                    PFN_vkDestroySwapchainKHR pfnDestroySwapchainKHR =
                        reinterpret_cast<PFN_vkDestroySwapchainKHR>(
                            entry().resolveDevice(sdevice, "vkDestroySwapchainKHR"));
                    if (pfnDestroySwapchainKHR != nullptr){
                        pfnDestroySwapchainKHR(sdevice, sswapchain, nullptr);
                    }
                    sswapchain = nullptr;
                }
                sdevice = nullptr;
                sextent = VkExtent2D{};
                simageCount = 0;
            }

            VkDevice sdevice = nullptr;
            VkSwapchainKHR sswapchain = nullptr;
            VkFormat sformat = VK_FORMAT_UNDEFINED;
            VkExtent2D sextent{};
            std::uint32_t simageCount = 0;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// swapchain dan image resource membutuhkan tipe lengkap dari partisi ini.
std::expected<renaru::Swapchain, renaru::Error>
renaru::Device::createSwapchain(std::uint32_t swidth, std::uint32_t sheight) const{
    if (sdevice == nullptr || ssurface == nullptr){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }

    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfnGetCapabilities =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
            entry().resolveInstance(sinstance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pfnGetFormats =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
            entry().resolveInstance(sinstance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pfnGetPresentModes =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
            entry().resolveInstance(sinstance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    if (pfnGetCapabilities == nullptr || pfnGetFormats == nullptr || pfnGetPresentModes == nullptr){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }

    VkSurfaceCapabilitiesKHR scapabilities{};
    if (pfnGetCapabilities(sphysicalDevice, ssurface, &scapabilities) != VK_SUCCESS){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }

    std::uint32_t sformatCount = 0;
    if (pfnGetFormats(sphysicalDevice, ssurface, &sformatCount, nullptr) != VK_SUCCESS ||
        sformatCount == 0){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }
    std::vector<VkSurfaceFormatKHR> sformats(sformatCount);
    if (pfnGetFormats(sphysicalDevice, ssurface, &sformatCount, sformats.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }

    std::uint32_t smodeCount = 0;
    if (pfnGetPresentModes(sphysicalDevice, ssurface, &smodeCount, nullptr) != VK_SUCCESS ||
        smodeCount == 0){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }
    std::vector<VkPresentModeKHR> smodes(smodeCount);
    if (pfnGetPresentModes(sphysicalDevice, ssurface, &smodeCount, smodes.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }

    VkSurfaceFormatKHR schosenFormat = sformats.front();
    for (const VkSurfaceFormatKHR& sformat : sformats){
        if (sformat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            sformat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            schosenFormat = sformat;
            break;
        }
    }

    VkPresentModeKHR schosenMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const VkPresentModeKHR smode : smodes){
        if (smode == VK_PRESENT_MODE_MAILBOX_KHR){
            schosenMode = smode;
            break;
        }
    }

    VkExtent2D sextent{};
    const std::uint32_t sundefinedExtent = (std::numeric_limits<std::uint32_t>::max)();
    if (scapabilities.currentExtent.width != sundefinedExtent &&
        scapabilities.currentExtent.height != sundefinedExtent){
        sextent = scapabilities.currentExtent;
    }else{
        sextent.width = std::clamp(swidth, scapabilities.minImageExtent.width,
                                   scapabilities.maxImageExtent.width);
        sextent.height = std::clamp(sheight, scapabilities.minImageExtent.height,
                                    scapabilities.maxImageExtent.height);
    }

    std::uint32_t simageCount = scapabilities.minImageCount + 1;
    if (scapabilities.maxImageCount > 0 && simageCount > scapabilities.maxImageCount){
        simageCount = scapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sinfo.surface = ssurface;
    sinfo.minImageCount = simageCount;
    sinfo.imageFormat = schosenFormat.format;
    sinfo.imageColorSpace = schosenFormat.colorSpace;
    sinfo.imageExtent = sextent;
    sinfo.imageArrayLayers = 1;
    sinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sinfo.preTransform = scapabilities.currentTransform;
    sinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sinfo.presentMode = schosenMode;
    sinfo.clipped = VK_TRUE;
    sinfo.oldSwapchain = nullptr;

    PFN_vkCreateSwapchainKHR pfnCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(
        entry().resolveDevice(sdevice, "vkCreateSwapchainKHR"));
    if (pfnCreateSwapchainKHR == nullptr){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }

    VkSwapchainKHR screated = nullptr;
    if (pfnCreateSwapchainKHR(sdevice, &sinfo, nullptr, &screated) != VK_SUCCESS ||
        screated == nullptr){
        return std::unexpected(renaru::Error::SwapchainCreationFailed);
    }

    std::println("Aru RHI: Swapchain(created!!) images={}", simageCount);

    registerSwapchain(screated, sextent, simageCount);

    return Swapchain(sdevice, screated, schosenFormat.format, sextent, simageCount);
}

std::expected<std::vector<renaru::Resource>, renaru::Error>
renaru::Device::createSwapchainImages(const renaru::Swapchain& sswapchain) const{
    if (sdevice == nullptr || sswapchain.empty()){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    PFN_vkGetSwapchainImagesKHR pfnGetSwapchainImagesKHR =
        reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(
            entry().resolveDevice(sdevice, "vkGetSwapchainImagesKHR"));
    PFN_vkCreateImageView pfnCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(
        entry().resolveDevice(sdevice, "vkCreateImageView"));
    if (pfnGetSwapchainImagesKHR == nullptr || pfnCreateImageView == nullptr){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    std::uint32_t scount = 0;
    if (pfnGetSwapchainImagesKHR(sdevice, sswapchain.handle(), &scount, nullptr) != VK_SUCCESS ||
        scount == 0){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }
    std::vector<VkImage> simages(scount);
    if (pfnGetSwapchainImagesKHR(sdevice, sswapchain.handle(), &scount, simages.data()) != VK_SUCCESS){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    std::vector<renaru::Resource> sresources;
    sresources.reserve(simages.size());

    for (const VkImage simage : simages){
        VkImageViewCreateInfo sinfo{};
        sinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        sinfo.image = simage;
        sinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        sinfo.format = sswapchain.format();
        sinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        sinfo.subresourceRange.baseMipLevel = 0;
        sinfo.subresourceRange.levelCount = 1;
        sinfo.subresourceRange.baseArrayLayer = 0;
        sinfo.subresourceRange.layerCount = 1;

        VkImageView sview = nullptr;
        if (pfnCreateImageView(sdevice, &sinfo, nullptr, &sview) != VK_SUCCESS ||
            sview == nullptr){
            return std::unexpected(renaru::Error::ResourceCreationFailed);
        }

        sresources.push_back(renaru::Resource(
            sdevice, simage, sview, sswapchain.extent()
        ));
    }

    std::println("Aru RHI: Swapchain images(image views ready) count={}", simages.size());

    return sresources;
}
