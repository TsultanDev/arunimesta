module;

#include <renaru/vulkan.hpp>

#include <array>
#include <expected>
#include <print>

#include <utility>

export module renaru:render_pass;

import :device;
import :swapchain;

export namespace renaru {
    class RenderPass {
        public:
            using ResultType = std::expected<RenderPass, Error>;

            // vk::RenderPass adalah raw handle: salinan tidak diizinkan.
            RenderPass(const RenderPass&) = delete;
            RenderPass& operator=(const RenderPass&) = delete;

            RenderPass(RenderPass&& other) noexcept :
                mOwner(std::exchange(other.mOwner, nullptr)),
                mDevice(std::exchange(other.mDevice, vk::Device{})),
                mRenderPass(std::exchange(other.mRenderPass, vk::RenderPass{})),
                mClearColor(std::exchange(other.mClearColor, std::array<float, 4>{}))
            {}

            RenderPass& operator=(RenderPass&& other) noexcept{
                if (this != &other){
                    this->release();
                    this->mOwner = std::exchange(other.mOwner, nullptr);
                    this->mDevice = std::exchange(other.mDevice, vk::Device{});
                    this->mRenderPass = std::exchange(other.mRenderPass, vk::RenderPass{});
                    this->mClearColor = std::exchange(other.mClearColor, std::array<float, 4>{});
                }
                return *this;
            }

            ~RenderPass(){ this->release(); }

            // Prototipe: render_pass.render([]{ ... }) memakai clear color
            // default; overload dengan array warna menerima RGBA float.
            template<typename Func>
            std::expected<void, Error> render(Func&& func) const{
                return this->renderImpl(mClearColor, std::forward<Func>(func));
            }

            template<typename Func>
            std::expected<void, Error> render(std::array<float, 4> clearColor, Func&& func) const{
                return this->renderImpl(clearColor, std::forward<Func>(func));
            }

            void setClearColor(std::array<float, 4> color) noexcept { this->mClearColor = color; }

            std::array<float, 4> clearColor() const noexcept { return this->mClearColor; }

            vk::RenderPass handle() const noexcept { return this->mRenderPass; }

            vk::Device deviceHandle() const noexcept { return this->mDevice; }

            bool empty() const noexcept { return static_cast<VkRenderPass>(this->mRenderPass) == nullptr; }

        private:
            friend class Device;

            RenderPass(const Device* owner, vk::Device device, vk::RenderPass renderPass) noexcept :
                mOwner(owner),
                mDevice(device),
                mRenderPass(renderPass),
                mClearColor({0.3F, 0.3F, 0.3F, 1.0F})
            {}

            template<typename Func>
            std::expected<void, Error> renderImpl(const std::array<float, 4>& clearColor,
                                                  Func&& func) const{
                if (auto started = this->beginRender(clearColor); !started){
                    return std::unexpected(started.error());
                }

                try{
                    std::forward<Func>(func)();
                }catch(...){
                    this->endRender();
                    const renaru::Error error = renaru::Error::RenderPassRecordingFailed;
                    if (mOwner != nullptr){
                        mOwner->noteError(error);
                    }
                    return std::unexpected(error);
                }

                this->endRender();
                if (mOwner != nullptr && mOwner->lastError() != Error::None){
                    return std::unexpected(mOwner->lastError());
                }
                return {};
            }

            std::expected<void, Error> beginRender(const std::array<float, 4>& clearColor) const{
                if (this->empty() || mOwner == nullptr){
                    return std::unexpected(renaru::Error::RenderPassRecordingFailed);
                }

                const vk::CommandBuffer commandBuffer = mOwner->activeCommandBuffer();
                const vk::Framebuffer framebuffer = mOwner->framebufferForCurrentImage();
                const vk::Extent2D extent = mOwner->frameExtent();
                if (static_cast<VkCommandBuffer>(commandBuffer) == nullptr ||
                    static_cast<VkFramebuffer>(framebuffer) == nullptr ||
                    extent.width == 0 || extent.height == 0){
                    return std::unexpected(renaru::Error::RenderPassRecordingFailed);
                }

                vk::ClearColorValue colorValue(clearColor);
                vk::ClearValue clear;
                clear.setColor(colorValue);

                vk::RenderPassBeginInfo info;
                info.setRenderPass(this->mRenderPass);
                info.setFramebuffer(framebuffer);
                info.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), extent));
                info.setClearValueCount(1);
                info.setPClearValues(&clear);

                try{
                    commandBuffer.beginRenderPass(info, vk::SubpassContents::eInline,
                                                  VULKAN_HPP_DEFAULT_DISPATCHER);
                }catch(...){
                    return std::unexpected(renaru::Error::RenderPassRecordingFailed);
                }
                return {};
            }

            void endRender() const noexcept{
                if (mOwner == nullptr){
                    return;
                }
                const vk::CommandBuffer commandBuffer = mOwner->activeCommandBuffer();
                if (static_cast<VkCommandBuffer>(commandBuffer) == nullptr){
                    return;
                }
                try{
                    commandBuffer.endRenderPass(VULKAN_HPP_DEFAULT_DISPATCHER);
                }catch(...){
                    mOwner->noteError(renaru::Error::RenderPassRecordingFailed);
                }
            }

            void release() noexcept{
                if (static_cast<VkRenderPass>(this->mRenderPass) != nullptr){
                    this->mDevice.destroyRenderPass(this->mRenderPass, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
                    this->mRenderPass = vk::RenderPass{};
                }
                this->mOwner = nullptr;
                this->mDevice = vk::Device{};
            }

            const Device* mOwner;
            vk::Device mDevice;
            vk::RenderPass mRenderPass;
            std::array<float, 4> mClearColor;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// RenderPass membutuhkan tipe lengkap Swapchain/RenderPass dari partisi ini.
std::expected<renaru::RenderPass, renaru::Error>
renaru::Device::createRenderPass(const renaru::Swapchain& swapchain) const{
    if (this->empty() || swapchain.empty()){
        return std::unexpected(renaru::Error::RenderPassCreationFailed);
    }

    vk::AttachmentDescription colorAttachment;
    colorAttachment.setFormat(swapchain.format());
    colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
    colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorReference;
    colorReference.setAttachment(0);
    colorReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachmentCount(1);
    subpass.setPColorAttachments(&colorReference);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    dependency.setDstSubpass(0);
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependency.setSrcAccessMask({});
    dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo info;
    info.setAttachmentCount(1);
    info.setPAttachments(&colorAttachment);
    info.setSubpassCount(1);
    info.setPSubpasses(&subpass);
    info.setDependencyCount(1);
    info.setPDependencies(&dependency);

    vk::RenderPass created;
    try{
        created = this->mDevice.createRenderPass(info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(renaru::Error::RenderPassCreationFailed);
    }

    std::println("Aru RHI: RenderPass(created!!)");

    return RenderPass(this, this->mDevice, created);
}
