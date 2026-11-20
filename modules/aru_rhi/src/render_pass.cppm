module;

#include <aru_rhi/vulkan.hpp>

#include <utility>

export module aru.rhi:render_pass;

import aru.standard;
import :device;
import :swapchain;

export namespace aru::rhi {
    class RenderPass;
}

export namespace aru::rhi {
    class RenderPass {
        public:
            using ResultType = aru::standard::Result<RenderPass, Error>;

            RenderPass(const RenderPass& other);
            RenderPass(RenderPass&& other) noexcept;
            RenderPass& operator=(const RenderPass& other);
            RenderPass& operator=(RenderPass&& other) noexcept;
            ~RenderPass();

            RenderPass copy() const;
            RenderPass move() noexcept;

            vk::RenderPass handle() const noexcept;
            vk::Device deviceHandle() const noexcept;
            bool empty() const noexcept;

        private:
            friend class Device;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            RenderPass(vk::Device device, vk::RenderPass renderPass, InitTag tag) noexcept;
            RenderPass(const RenderPass& other, CopyTag tag) noexcept;
            RenderPass(RenderPass&& other, MoveTag tag) noexcept;

            void release() noexcept;

            vk::Device mDevice;
            vk::RenderPass mRenderPass;
    };
}

aru::standard::Result<aru::rhi::RenderPass, aru::rhi::Error>
aru::rhi::Device::createRenderPass(const aru::rhi::Swapchain& swapchain) const{
    if (this->empty() || swapchain.empty()){
        return aru::rhi::Error::RenderPassCreationFailed;
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
        return aru::rhi::Error::RenderPassCreationFailed;
    }

    aru::standard::println("Aru RHI: RenderPass(created!!)");

    return RenderPass(this->mDevice, created, RenderPass::InitTag{});
}

aru::rhi::RenderPass::RenderPass(vk::Device device, vk::RenderPass renderPass, InitTag) noexcept :
    mDevice(device),
    mRenderPass(renderPass)
{}

aru::rhi::RenderPass::RenderPass(const RenderPass& other) :
    RenderPass(other, CopyTag{})
{}

aru::rhi::RenderPass::RenderPass(RenderPass&& other) noexcept :
    RenderPass(std::move(other), MoveTag{})
{}

aru::rhi::RenderPass::RenderPass(const RenderPass& other, CopyTag) noexcept :
    mDevice(other.mDevice),
    mRenderPass()
{}

aru::rhi::RenderPass::RenderPass(RenderPass&& other, MoveTag) noexcept :
    mDevice(other.mDevice),
    mRenderPass(std::exchange(other.mRenderPass, vk::RenderPass{}))
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
        this->mDevice = other.mDevice;
        this->mRenderPass = std::exchange(other.mRenderPass, vk::RenderPass{});
    }
    return *this;
}

aru::rhi::RenderPass aru::rhi::RenderPass::copy() const{
    return RenderPass(*this, CopyTag{});
}

aru::rhi::RenderPass aru::rhi::RenderPass::move() noexcept{
    return RenderPass(std::move(*this), MoveTag{});
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
}

aru::rhi::RenderPass::~RenderPass(){
    this->release();
}
