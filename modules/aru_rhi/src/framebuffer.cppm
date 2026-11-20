module;

#include <aru_rhi/vulkan.hpp>

#include <cstdint>
#include <utility>

export module aru.rhi:framebuffer;

import aru.standard;
import :device;
import :resource;
import :render_pass;

export namespace aru::rhi {
    class Framebuffer;
}

export namespace aru::rhi {
    class Framebuffer {
        public:
            using ResultType = aru::standard::Result<Framebuffer, Error>;

            Framebuffer(const Framebuffer& other);
            Framebuffer(Framebuffer&& other) noexcept;
            Framebuffer& operator=(const Framebuffer& other);
            Framebuffer& operator=(Framebuffer&& other) noexcept;
            ~Framebuffer();

            Framebuffer copy() const;
            Framebuffer move() noexcept;

            vk::Framebuffer handle() const noexcept;
            vk::Device deviceHandle() const noexcept;
            std::uint32_t width() const noexcept;
            std::uint32_t height() const noexcept;
            bool empty() const noexcept;

        private:
            friend class Device;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Framebuffer(vk::Device device, vk::Framebuffer framebuffer,
                        std::uint32_t width, std::uint32_t height, InitTag tag) noexcept;
            Framebuffer(const Framebuffer& other, CopyTag tag) noexcept;
            Framebuffer(Framebuffer&& other, MoveTag tag) noexcept;

            void release() noexcept;

            vk::Device mDevice;
            vk::Framebuffer mFramebuffer;
            std::uint32_t mWidth;
            std::uint32_t mHeight;
    };
}

aru::standard::Result<aru::rhi::Framebuffer, aru::rhi::Error>
aru::rhi::Device::createFramebuffer(const aru::rhi::RenderPass& renderPass,
                                    const aru::rhi::Resource& image) const{
    if (this->empty() || renderPass.empty() || image.empty()){
        return aru::rhi::Error::FramebufferCreationFailed;
    }
    if (image.kind() != aru::rhi::ResourceKind::Image){
        return aru::rhi::Error::FramebufferCreationFailed;
    }

    const vk::ImageView attachment = image.imageViewHandle();
    if (static_cast<VkImageView>(attachment) == nullptr){
        return aru::rhi::Error::FramebufferCreationFailed;
    }

    const vk::Extent2D extent = image.extent();
    if (extent.width == 0 || extent.height == 0){
        return aru::rhi::Error::FramebufferCreationFailed;
    }

    vk::FramebufferCreateInfo info;
    info.setRenderPass(renderPass.handle());
    info.setAttachmentCount(1);
    info.setPAttachments(&attachment);
    info.setWidth(extent.width);
    info.setHeight(extent.height);
    info.setLayers(1);

    vk::Framebuffer created;
    try{
        created = this->mDevice.createFramebuffer(info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return aru::rhi::Error::FramebufferCreationFailed;
    }

    return Framebuffer(this->mDevice, created, extent.width, extent.height, Framebuffer::InitTag{});
}

aru::rhi::Framebuffer::Framebuffer(vk::Device device, vk::Framebuffer framebuffer,
                                   std::uint32_t width, std::uint32_t height, InitTag) noexcept :
    mDevice(device),
    mFramebuffer(framebuffer),
    mWidth(width),
    mHeight(height)
{}

aru::rhi::Framebuffer::Framebuffer(const Framebuffer& other) :
    Framebuffer(other, CopyTag{})
{}

aru::rhi::Framebuffer::Framebuffer(Framebuffer&& other) noexcept :
    Framebuffer(std::move(other), MoveTag{})
{}

aru::rhi::Framebuffer::Framebuffer(const Framebuffer& other, CopyTag) noexcept :
    mDevice(other.mDevice),
    mFramebuffer(),
    mWidth(other.mWidth),
    mHeight(other.mHeight)
{}

aru::rhi::Framebuffer::Framebuffer(Framebuffer&& other, MoveTag) noexcept :
    mDevice(other.mDevice),
    mFramebuffer(std::exchange(other.mFramebuffer, vk::Framebuffer{})),
    mWidth(std::exchange(other.mWidth, std::uint32_t{0})),
    mHeight(std::exchange(other.mHeight, std::uint32_t{0}))
{}

aru::rhi::Framebuffer& aru::rhi::Framebuffer::operator=(const Framebuffer& other){
    if (this != &other){
        Framebuffer fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::Framebuffer& aru::rhi::Framebuffer::operator=(Framebuffer&& other) noexcept{
    if (this != &other){
        this->release();
        this->mDevice = other.mDevice;
        this->mFramebuffer = std::exchange(other.mFramebuffer, vk::Framebuffer{});
        this->mWidth = std::exchange(other.mWidth, std::uint32_t{0});
        this->mHeight = std::exchange(other.mHeight, std::uint32_t{0});
    }
    return *this;
}

aru::rhi::Framebuffer aru::rhi::Framebuffer::copy() const{
    return Framebuffer(*this, CopyTag{});
}

aru::rhi::Framebuffer aru::rhi::Framebuffer::move() noexcept{
    return Framebuffer(std::move(*this), MoveTag{});
}

vk::Framebuffer aru::rhi::Framebuffer::handle() const noexcept{
    return this->mFramebuffer;
}

vk::Device aru::rhi::Framebuffer::deviceHandle() const noexcept{
    return this->mDevice;
}

std::uint32_t aru::rhi::Framebuffer::width() const noexcept{
    return this->mWidth;
}

std::uint32_t aru::rhi::Framebuffer::height() const noexcept{
    return this->mHeight;
}

bool aru::rhi::Framebuffer::empty() const noexcept{
    return static_cast<VkFramebuffer>(this->mFramebuffer) == nullptr;
}

void aru::rhi::Framebuffer::release() noexcept{
    if (static_cast<VkFramebuffer>(this->mFramebuffer) != nullptr){
        this->mDevice.destroyFramebuffer(this->mFramebuffer, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        this->mFramebuffer = vk::Framebuffer{};
    }
    this->mWidth = 0;
    this->mHeight = 0;
}

aru::rhi::Framebuffer::~Framebuffer(){
    this->release();
}
