module;

#include <aru_rhi/vulkan.hpp>

#include <expected>

#include <cstdint>
#include <utility>

export module aru.rhi:framebuffer;

import :device;
import :resource;
import :render_pass;

export namespace aru::rhi {
    class Framebuffer {
        private:
            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

        public:
            using ResultType = std::expected<Framebuffer, Error>;

            Framebuffer(const Framebuffer& other) : Framebuffer(other, CopyTag{}) {}

            Framebuffer(Framebuffer&& other) noexcept : Framebuffer(std::move(other), MoveTag{}) {}

            Framebuffer& operator=(const Framebuffer& other){
                if (this != &other){
                    Framebuffer fresh(other, CopyTag{});
                    *this = std::move(fresh);
                }
                return *this;
            }

            Framebuffer& operator=(Framebuffer&& other) noexcept{
                if (this != &other){
                    this->release();
                    this->mDevice = other.mDevice;
                    this->mFramebuffer = std::exchange(other.mFramebuffer, vk::Framebuffer{});
                    this->mWidth = std::exchange(other.mWidth, std::uint32_t{0});
                    this->mHeight = std::exchange(other.mHeight, std::uint32_t{0});
                }
                return *this;
            }

            ~Framebuffer(){ this->release(); }

            Framebuffer copy() const { return Framebuffer(*this, CopyTag{}); }

            Framebuffer move() noexcept { return Framebuffer(std::move(*this), MoveTag{}); }

            vk::Framebuffer handle() const noexcept { return this->mFramebuffer; }

            vk::Device deviceHandle() const noexcept { return this->mDevice; }

            std::uint32_t width() const noexcept { return this->mWidth; }

            std::uint32_t height() const noexcept { return this->mHeight; }

            bool empty() const noexcept { return static_cast<VkFramebuffer>(this->mFramebuffer) == nullptr; }

        private:
            friend class Device;

            Framebuffer(vk::Device device, vk::Framebuffer framebuffer,
                        std::uint32_t width, std::uint32_t height, InitTag) noexcept :
                mDevice(device),
                mFramebuffer(framebuffer),
                mWidth(width),
                mHeight(height)
            {}

            Framebuffer(const Framebuffer& other, CopyTag) noexcept :
                mDevice(other.mDevice),
                mFramebuffer(),
                mWidth(other.mWidth),
                mHeight(other.mHeight)
            {}

            Framebuffer(Framebuffer&& other, MoveTag) noexcept :
                mDevice(other.mDevice),
                mFramebuffer(std::exchange(other.mFramebuffer, vk::Framebuffer{})),
                mWidth(std::exchange(other.mWidth, std::uint32_t{0})),
                mHeight(std::exchange(other.mHeight, std::uint32_t{0}))
            {}

            void release() noexcept{
                if (static_cast<VkFramebuffer>(this->mFramebuffer) != nullptr){
                    this->mDevice.destroyFramebuffer(this->mFramebuffer, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
                    this->mFramebuffer = vk::Framebuffer{};
                }
                this->mWidth = 0;
                this->mHeight = 0;
            }

            vk::Device mDevice;
            vk::Framebuffer mFramebuffer;
            std::uint32_t mWidth;
            std::uint32_t mHeight;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// Framebuffer membutuhkan tipe lengkap RenderPass/Resource/Framebuffer di sini.
std::expected<aru::rhi::Framebuffer, aru::rhi::Error>
aru::rhi::Device::createFramebuffer(const aru::rhi::RenderPass& renderPass,
                                    const aru::rhi::Resource& image) const{
    if (this->empty() || renderPass.empty() || image.empty()){
        return std::unexpected(aru::rhi::Error::FramebufferCreationFailed);
    }
    if (image.kind() != aru::rhi::ResourceKind::Image){
        return std::unexpected(aru::rhi::Error::FramebufferCreationFailed);
    }

    const vk::ImageView attachment = image.imageViewHandle();
    if (static_cast<VkImageView>(attachment) == nullptr){
        return std::unexpected(aru::rhi::Error::FramebufferCreationFailed);
    }

    const vk::Extent2D extent = image.extent();
    if (extent.width == 0 || extent.height == 0){
        return std::unexpected(aru::rhi::Error::FramebufferCreationFailed);
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
        return std::unexpected(aru::rhi::Error::FramebufferCreationFailed);
    }

    this->registerFramebuffer(created);

    return Framebuffer(this->mDevice, created, extent.width, extent.height, Framebuffer::InitTag{});
}
