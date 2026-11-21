module;

#include <aru_rhi/vulkan.hpp>

#include <array>
#include <expected>
#include <print>

#include <utility>

export module aru.rhi:render_pass;

import :device;
import :swapchain;

export namespace aru::rhi {
    class RenderPass;
}

export namespace aru::rhi {
    class RenderPass {
        public:
            using ResultType = std::expected<RenderPass, Error>;

            RenderPass(const RenderPass& other);
            RenderPass(RenderPass&& other) noexcept;
            RenderPass& operator=(const RenderPass& other);
            RenderPass& operator=(RenderPass&& other) noexcept;
            ~RenderPass();

            RenderPass copy() const;
            RenderPass move() noexcept;

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

            void setClearColor(std::array<float, 4> color) noexcept;
            std::array<float, 4> clearColor() const noexcept;

            vk::RenderPass handle() const noexcept;
            vk::Device deviceHandle() const noexcept;
            bool empty() const noexcept;

        private:
            friend class Device;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            RenderPass(const Device* owner, vk::Device device, vk::RenderPass renderPass,
                       InitTag tag) noexcept;
            RenderPass(const RenderPass& other, CopyTag tag) noexcept;
            RenderPass(RenderPass&& other, MoveTag tag) noexcept;

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
                    const aru::rhi::Error error = aru::rhi::Error::RenderPassRecordingFailed;
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

            std::expected<void, Error> beginRender(const std::array<float, 4>& clearColor) const;
            void endRender() const noexcept;

            void release() noexcept;

            const Device* mOwner;
            vk::Device mDevice;
            vk::RenderPass mRenderPass;
            std::array<float, 4> mClearColor;
    };
}

std::expected<aru::rhi::RenderPass, aru::rhi::Error>
aru::rhi::Device::createRenderPass(const aru::rhi::Swapchain& swapchain) const{
    if (this->empty() || swapchain.empty()){
        return std::unexpected(aru::rhi::Error::RenderPassCreationFailed);
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
        return std::unexpected(aru::rhi::Error::RenderPassCreationFailed);
    }

    std::println("Aru RHI: RenderPass(created!!)");

    return RenderPass(this, this->mDevice, created, RenderPass::InitTag{});
}

aru::rhi::RenderPass::RenderPass(const Device* owner, vk::Device device,
                                 vk::RenderPass renderPass, InitTag) noexcept :
    mOwner(owner),
    mDevice(device),
    mRenderPass(renderPass),
    mClearColor({0.3F, 0.3F, 0.3F, 1.0F})
{}

aru::rhi::RenderPass::RenderPass(const RenderPass& other) :
    RenderPass(other, CopyTag{})
{}

aru::rhi::RenderPass::RenderPass(RenderPass&& other) noexcept :
    RenderPass(std::move(other), MoveTag{})
{}

aru::rhi::RenderPass::RenderPass(const RenderPass& other, CopyTag) noexcept :
    mOwner(other.mOwner),
    mDevice(other.mDevice),
    mRenderPass(),
    mClearColor(other.mClearColor)
{}

aru::rhi::RenderPass::RenderPass(RenderPass&& other, MoveTag) noexcept :
    mOwner(std::exchange(other.mOwner, nullptr)),
    mDevice(other.mDevice),
    mRenderPass(std::exchange(other.mRenderPass, vk::RenderPass{})),
    mClearColor(std::exchange(other.mClearColor, std::array<float, 4>{}))
{}

aru::rhi::RenderPass& aru::rhi::RenderPass::operator=(const RenderPass& other){
    if (this != &other){
        RenderPass fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::RenderPass& aru::rhi::RenderPass::operator=(RenderPass&& other) noexcept{
    if (this != &other){
        this->release();
        this->mOwner = std::exchange(other.mOwner, nullptr);
        this->mDevice = other.mDevice;
        this->mRenderPass = std::exchange(other.mRenderPass, vk::RenderPass{});
        this->mClearColor = std::exchange(other.mClearColor, std::array<float, 4>{});
    }
    return *this;
}

aru::rhi::RenderPass aru::rhi::RenderPass::copy() const{
    return RenderPass(*this, CopyTag{});
}

aru::rhi::RenderPass aru::rhi::RenderPass::move() noexcept{
    return RenderPass(std::move(*this), MoveTag{});
}

void aru::rhi::RenderPass::setClearColor(std::array<float, 4> color) noexcept{
    this->mClearColor = color;
}

std::array<float, 4> aru::rhi::RenderPass::clearColor() const noexcept{
    return this->mClearColor;
}

std::expected<void, aru::rhi::Error>
aru::rhi::RenderPass::beginRender(const std::array<float, 4>& clearColor) const{
    if (this->empty() || mOwner == nullptr){
        return std::unexpected(aru::rhi::Error::RenderPassRecordingFailed);
    }

    const vk::CommandBuffer commandBuffer = mOwner->activeCommandBuffer();
    const vk::Framebuffer framebuffer = mOwner->framebufferForCurrentImage();
    const vk::Extent2D extent = mOwner->frameExtent();
    if (static_cast<VkCommandBuffer>(commandBuffer) == nullptr ||
        static_cast<VkFramebuffer>(framebuffer) == nullptr ||
        extent.width == 0 || extent.height == 0){
        return std::unexpected(aru::rhi::Error::RenderPassRecordingFailed);
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
        return std::unexpected(aru::rhi::Error::RenderPassRecordingFailed);
    }
    return {};
}

void aru::rhi::RenderPass::endRender() const noexcept{
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
        mOwner->noteError(aru::rhi::Error::RenderPassRecordingFailed);
    }
}

vk::RenderPass aru::rhi::RenderPass::handle() const noexcept{
    return this->mRenderPass;
}

vk::Device aru::rhi::RenderPass::deviceHandle() const noexcept{
    return this->mDevice;
}

bool aru::rhi::RenderPass::empty() const noexcept{
    return static_cast<VkRenderPass>(this->mRenderPass) == nullptr;
}

void aru::rhi::RenderPass::release() noexcept{
    if (static_cast<VkRenderPass>(this->mRenderPass) != nullptr){
        this->mDevice.destroyRenderPass(this->mRenderPass, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        this->mRenderPass = vk::RenderPass{};
    }
    this->mOwner = nullptr;
    this->mDevice = vk::Device{};
}

aru::rhi::RenderPass::~RenderPass(){
    this->release();
}
