module;

#include <aru_rhi/vulkan.hpp>

#include <algorithm>
#include <cstdint>
#include <expected>
#include <limits>
#include <print>
#include <utility>
#include <vector>

export module aru.rhi:swapchain;

import :device;
import :resource;

export namespace aru::rhi {
    class Swapchain;
}

export namespace aru::rhi {
    class Swapchain {
        public:
            using ResultType = std::expected<Swapchain, Error>;

            Swapchain(const Swapchain& other);
            Swapchain(Swapchain&& other) noexcept;
            Swapchain& operator=(const Swapchain& other);
            Swapchain& operator=(Swapchain&& other) noexcept;
            ~Swapchain();

            Swapchain copy() const;
            Swapchain move() noexcept;

            vk::SwapchainKHR handle() const noexcept;
            vk::Device deviceHandle() const noexcept;
            vk::Format format() const noexcept;
            vk::Extent2D extent() const noexcept;
            std::uint32_t imageCount() const noexcept;
            bool empty() const noexcept;

        private:
            friend class Device;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Swapchain(vk::Device device, vk::SwapchainKHR swapchain,
                      vk::Format format, vk::Extent2D extent,
                      std::uint32_t imageCount, InitTag tag) noexcept;
            Swapchain(const Swapchain& other, CopyTag tag) noexcept;
            Swapchain(Swapchain&& other, MoveTag tag) noexcept;

            void release() noexcept;

            vk::Device mDevice;
            vk::SwapchainKHR mSwapchain;
            vk::Format mFormat;
            vk::Extent2D mExtent;
            std::uint32_t mImageCount;
    };
}

std::expected<aru::rhi::Swapchain, aru::rhi::Error>
aru::rhi::Device::createSwapchain(std::uint32_t width, std::uint32_t height) const{
    if (this->empty() || static_cast<VkSurfaceKHR>(this->mSurface) == nullptr){
        return std::unexpected(aru::rhi::Error::SwapchainCreationFailed);
    }

    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
    try{
        capabilities = this->mPhysicalDevice.getSurfaceCapabilitiesKHR(
            this->mSurface, VULKAN_HPP_DEFAULT_DISPATCHER
        );
        formats = this->mPhysicalDevice.getSurfaceFormatsKHR(
            this->mSurface, VULKAN_HPP_DEFAULT_DISPATCHER
        );
        presentModes = this->mPhysicalDevice.getSurfacePresentModesKHR(
            this->mSurface, VULKAN_HPP_DEFAULT_DISPATCHER
        );
    }catch(...){
        return std::unexpected(aru::rhi::Error::SwapchainCreationFailed);
    }

    if (formats.empty() || presentModes.empty()){
        return std::unexpected(aru::rhi::Error::SwapchainCreationFailed);
    }

    vk::SurfaceFormatKHR chosenFormat = formats.front();
    for (const vk::SurfaceFormatKHR& format : formats){
        if (format.format == vk::Format::eB8G8R8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear){
            chosenFormat = format;
            break;
        }
    }

    vk::PresentModeKHR chosenMode = vk::PresentModeKHR::eFifo;
    for (const vk::PresentModeKHR mode : presentModes){
        if (mode == vk::PresentModeKHR::eMailbox){
            chosenMode = mode;
            break;
        }
    }

    vk::Extent2D extent;
    const std::uint32_t undefinedExtent = (std::numeric_limits<std::uint32_t>::max)();
    if (capabilities.currentExtent.width != undefinedExtent &&
        capabilities.currentExtent.height != undefinedExtent){
        extent = capabilities.currentExtent;
    }else{
        extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    std::uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount){
        imageCount = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR info;
    info.setSurface(this->mSurface);
    info.setMinImageCount(imageCount);
    info.setImageFormat(chosenFormat.format);
    info.setImageColorSpace(chosenFormat.colorSpace);
    info.setImageExtent(extent);
    info.setImageArrayLayers(1);
    info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    info.setImageSharingMode(vk::SharingMode::eExclusive);
    info.setPreTransform(capabilities.currentTransform);
    info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    info.setPresentMode(chosenMode);
    info.setClipped(VK_TRUE);
    info.setOldSwapchain(vk::SwapchainKHR{});

    vk::SwapchainKHR created;
    try{
        created = this->mDevice.createSwapchainKHR(info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(aru::rhi::Error::SwapchainCreationFailed);
    }

    std::println("Aru RHI: Swapchain(created!!) images={}", imageCount);

    return Swapchain(this->mDevice, created, chosenFormat.format, extent, imageCount, Swapchain::InitTag{});
}

std::expected<std::vector<aru::rhi::Resource>, aru::rhi::Error>
aru::rhi::Device::createSwapchainImages(const aru::rhi::Swapchain& swapchain) const{
    if (this->empty() || swapchain.empty()){
        return std::unexpected(aru::rhi::Error::ResourceCreationFailed);
    }

    std::vector<vk::Image> images;
    try{
        images = this->mDevice.getSwapchainImagesKHR(
            swapchain.handle(), VULKAN_HPP_DEFAULT_DISPATCHER
        );
    }catch(...){
        return std::unexpected(aru::rhi::Error::ResourceCreationFailed);
    }

    std::vector<aru::rhi::Resource> resources;
    resources.reserve(images.size());

    for (const vk::Image image : images){
        vk::ImageViewCreateInfo info;
        info.setImage(image);
        info.setViewType(vk::ImageViewType::e2D);
        info.setFormat(swapchain.format());

        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor);
        range.setBaseMipLevel(0);
        range.setLevelCount(1);
        range.setBaseArrayLayer(0);
        range.setLayerCount(1);
        info.setSubresourceRange(range);

        vk::ImageView view;
        try{
            view = this->mDevice.createImageView(info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        }catch(...){
            return std::unexpected(aru::rhi::Error::ResourceCreationFailed);
        }

        resources.push_back(aru::rhi::Resource(
            this->mDevice, image, view, swapchain.extent(), aru::rhi::Resource::InitTag{}
        ));
    }

    std::println("Aru RHI: Swapchain images(image views ready) count={}", images.size());

    return resources;
}

aru::rhi::Swapchain::Swapchain(vk::Device device, vk::SwapchainKHR swapchain,
                               vk::Format format, vk::Extent2D extent,
                               std::uint32_t imageCount, InitTag) noexcept :
    mDevice(device),
    mSwapchain(swapchain),
    mFormat(format),
    mExtent(extent),
    mImageCount(imageCount)
{}

aru::rhi::Swapchain::Swapchain(const Swapchain& other) :
    Swapchain(other, CopyTag{})
{}

aru::rhi::Swapchain::Swapchain(Swapchain&& other) noexcept :
    Swapchain(std::move(other), MoveTag{})
{}

aru::rhi::Swapchain::Swapchain(const Swapchain& other, CopyTag) noexcept :
    mDevice(other.mDevice),
    mSwapchain(),
    mFormat(other.mFormat),
    mExtent(other.mExtent),
    mImageCount(other.mImageCount)
{}

aru::rhi::Swapchain::Swapchain(Swapchain&& other, MoveTag) noexcept :
    mDevice(other.mDevice),
    mSwapchain(std::exchange(other.mSwapchain, vk::SwapchainKHR{})),
    mFormat(other.mFormat),
    mExtent(other.mExtent),
    mImageCount(std::exchange(other.mImageCount, std::uint32_t{0}))
{}

aru::rhi::Swapchain& aru::rhi::Swapchain::operator=(const Swapchain& other){
    if (this != &other){
        Swapchain fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::Swapchain& aru::rhi::Swapchain::operator=(Swapchain&& other) noexcept{
    if (this != &other){
        this->release();
        this->mDevice = other.mDevice;
        this->mSwapchain = std::exchange(other.mSwapchain, vk::SwapchainKHR{});
        this->mFormat = other.mFormat;
        this->mExtent = other.mExtent;
        this->mImageCount = std::exchange(other.mImageCount, std::uint32_t{0});
    }
    return *this;
}

aru::rhi::Swapchain aru::rhi::Swapchain::copy() const{
    return Swapchain(*this, CopyTag{});
}

aru::rhi::Swapchain aru::rhi::Swapchain::move() noexcept{
    return Swapchain(std::move(*this), MoveTag{});
}

vk::SwapchainKHR aru::rhi::Swapchain::handle() const noexcept{
    return this->mSwapchain;
}

vk::Device aru::rhi::Swapchain::deviceHandle() const noexcept{
    return this->mDevice;
}

vk::Format aru::rhi::Swapchain::format() const noexcept{
    return this->mFormat;
}

vk::Extent2D aru::rhi::Swapchain::extent() const noexcept{
    return this->mExtent;
}

std::uint32_t aru::rhi::Swapchain::imageCount() const noexcept{
    return this->mImageCount;
}

bool aru::rhi::Swapchain::empty() const noexcept{
    return static_cast<VkSwapchainKHR>(this->mSwapchain) == nullptr;
}

void aru::rhi::Swapchain::release() noexcept{
    if (static_cast<VkSwapchainKHR>(this->mSwapchain) != nullptr){
        this->mDevice.destroySwapchainKHR(this->mSwapchain, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        this->mSwapchain = vk::SwapchainKHR{};
    }
    this->mImageCount = 0;
}

aru::rhi::Swapchain::~Swapchain(){
    this->release();
}
