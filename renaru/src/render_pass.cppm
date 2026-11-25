module;

#include <vulkan/vulkan.h>

#include <array>
#include <expected>
#include <print>
#include <utility>

export module renaru:render_pass;

import :entry;
import :device;
import :swapchain;

export namespace renaru {
    class RenderPass {
        private:
            using ResultType = std::expected<RenderPass, Error>;

            const Device* sOwner = nullptr;
            VkDevice sDevice = nullptr;
            VkRenderPass sRenderPass = nullptr;
            std::array<float, 4> sClearColor{0.3F, 0.3F, 0.3F, 1.0F};

            RenderPass(const Device* sOwner, VkDevice sDevice, VkRenderPass sRenderPass) noexcept :
                sOwner(sOwner),
                sDevice(sDevice),
                sRenderPass(sRenderPass)
            {}

            void sDestroy() noexcept{
                if (sRenderPass != nullptr && sDevice != nullptr){
                    PFN_vkDestroyRenderPass pfnDestroyRenderPass =
                        reinterpret_cast<PFN_vkDestroyRenderPass>(
                            entry().resolveDevice(sDevice, "vkDestroyRenderPass"));
                    if (pfnDestroyRenderPass != nullptr){
                        pfnDestroyRenderPass(sDevice, sRenderPass, nullptr);
                    }
                    sRenderPass = nullptr;
                }
                sOwner = nullptr;
                sDevice = nullptr;
            }

            template<typename Func>
            std::expected<void, Error> sRenderImpl(const std::array<float, 4>& sClearColor,
                                                   Func&& sFunc) const{
                if (auto sStarted = sBeginRender(sClearColor); !sStarted){
                    return std::unexpected(sStarted.error());
                }

                try{
                    std::forward<Func>(sFunc)();
                }catch(...){
                    sEndRender();
                    const renaru::Error sError = renaru::Error::eRenderPassRecordingFailed;
                    if (sOwner != nullptr){
                        sOwner->sNoteError(sError);
                    }
                    return std::unexpected(sError);
                }

                sEndRender();
                if (sOwner != nullptr && sOwner->sCurrentError() != Error::eNone){
                    return std::unexpected(sOwner->sCurrentError());
                }
                return {};
            }

            std::expected<void, Error> sBeginRender(const std::array<float, 4>& sClearColor) const{
                if (sRenderPass == nullptr || sOwner == nullptr){
                    return std::unexpected(renaru::Error::eRenderPassRecordingFailed);
                }

                const VkCommandBuffer sCommandBuffer = sOwner->sActiveCommandBuffer;
                const VkFramebuffer sFramebuffer = sOwner->sFramebufferForCurrentImage();
                const VkExtent2D sExtent = sOwner->sCurrentFrameExtent();
                if (sCommandBuffer == nullptr || sFramebuffer == nullptr ||
                    sExtent.width == 0 || sExtent.height == 0){
                    return std::unexpected(renaru::Error::eRenderPassRecordingFailed);
                }

                VkClearValue sClear{};
                sClear.color.float32[0] = sClearColor[0];
                sClear.color.float32[1] = sClearColor[1];
                sClear.color.float32[2] = sClearColor[2];
                sClear.color.float32[3] = sClearColor[3];

                VkRenderPassBeginInfo sInfo{};
                sInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                sInfo.renderPass = sRenderPass;
                sInfo.framebuffer = sFramebuffer;
                sInfo.renderArea.offset = {0, 0};
                sInfo.renderArea.extent = sExtent;
                sInfo.clearValueCount = 1;
                sInfo.pClearValues = &sClear;

                PFN_vkCmdBeginRenderPass pfnCmdBeginRenderPass =
                    reinterpret_cast<PFN_vkCmdBeginRenderPass>(
                        entry().resolveDevice(sDevice, "vkCmdBeginRenderPass"));
                if (pfnCmdBeginRenderPass == nullptr){
                    return std::unexpected(renaru::Error::eRenderPassRecordingFailed);
                }
                pfnCmdBeginRenderPass(sCommandBuffer, &sInfo, VK_SUBPASS_CONTENTS_INLINE);
                return {};
            }

            void sEndRender() const noexcept{
                if (sOwner == nullptr){
                    return;
                }
                const VkCommandBuffer sCommandBuffer = sOwner->sActiveCommandBuffer;
                if (sCommandBuffer == nullptr){
                    return;
                }
                PFN_vkCmdEndRenderPass pfnCmdEndRenderPass =
                    reinterpret_cast<PFN_vkCmdEndRenderPass>(
                        entry().resolveDevice(sDevice, "vkCmdEndRenderPass"));
                if (pfnCmdEndRenderPass == nullptr){
                    sOwner->sNoteError(renaru::Error::eRenderPassRecordingFailed);
                    return;
                }
                pfnCmdEndRenderPass(sCommandBuffer);
            }

            friend class Device;

        public:
            RenderPass(const RenderPass&) = delete;
            RenderPass& operator=(const RenderPass&) = delete;

            RenderPass(RenderPass&& sOther) noexcept :
                sOwner(std::exchange(sOther.sOwner, nullptr)),
                sDevice(std::exchange(sOther.sDevice, nullptr)),
                sRenderPass(std::exchange(sOther.sRenderPass, nullptr)),
                sClearColor(std::exchange(sOther.sClearColor, std::array<float, 4>{}))
            {}

            RenderPass& operator=(RenderPass&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sOwner = std::exchange(sOther.sOwner, nullptr);
                    sDevice = std::exchange(sOther.sDevice, nullptr);
                    sRenderPass = std::exchange(sOther.sRenderPass, nullptr);
                    sClearColor = std::exchange(sOther.sClearColor, std::array<float, 4>{});
                }
                return *this;
            }

            ~RenderPass(){ sDestroy(); }

            template<typename Func>
            std::expected<void, Error> render(Func&& sFunc) const{
                return sRenderImpl(sClearColor, std::forward<Func>(sFunc));
            }

            template<typename Func>
            std::expected<void, Error> render(std::array<float, 4> sClearColor,
                                              Func&& sFunc) const{
                return sRenderImpl(sClearColor, std::forward<Func>(sFunc));
            }

            void setClearColor(std::array<float, 4> sColor) noexcept{
                sClearColor = sColor;
            }
    };
}

std::expected<renaru::RenderPass, renaru::Error>
renaru::Device::createRenderPass(const renaru::Swapchain& sSwapchain) const{
    if (sDevice == nullptr || sSwapchain.sSwapchain == nullptr){
        return std::unexpected(renaru::Error::eRenderPassCreationFailed);
    }

    VkAttachmentDescription sColorAttachment{};
    sColorAttachment.format = sSwapchain.sFormat;
    sColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    sColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    sColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    sColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    sColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    sColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    sColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference sColorReference{};
    sColorReference.attachment = 0;
    sColorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sSubpass{};
    sSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sSubpass.colorAttachmentCount = 1;
    sSubpass.pColorAttachments = &sColorReference;

    VkSubpassDependency sDependency{};
    sDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    sDependency.dstSubpass = 0;
    sDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sDependency.srcAccessMask = 0;
    sDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    sInfo.attachmentCount = 1;
    sInfo.pAttachments = &sColorAttachment;
    sInfo.subpassCount = 1;
    sInfo.pSubpasses = &sSubpass;
    sInfo.dependencyCount = 1;
    sInfo.pDependencies = &sDependency;

    PFN_vkCreateRenderPass pfnCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(
        entry().resolveDevice(sDevice, "vkCreateRenderPass"));
    if (pfnCreateRenderPass == nullptr){
        return std::unexpected(renaru::Error::eRenderPassCreationFailed);
    }

    VkRenderPass sCreated = nullptr;
    if (pfnCreateRenderPass(sDevice, &sInfo, nullptr, &sCreated) != VK_SUCCESS ||
        sCreated == nullptr){
        return std::unexpected(renaru::Error::eRenderPassCreationFailed);
    }

    std::println("Aru RHI: RenderPass(created!!)");

    return RenderPass(this, sDevice, sCreated);
}
