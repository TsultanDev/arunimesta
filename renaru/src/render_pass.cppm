module;

#include <renaru/vulkan.h>

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
        public:
            using ResultType = std::expected<RenderPass, Error>;

            // VkRenderPass adalah raw handle: salinan tidak diizinkan.
            RenderPass(const RenderPass&) = delete;
            RenderPass& operator=(const RenderPass&) = delete;

            RenderPass(RenderPass&& sother) noexcept :
                sowner(std::exchange(sother.sowner, nullptr)),
                sdevice(std::exchange(sother.sdevice, nullptr)),
                srenderPass(std::exchange(sother.srenderPass, nullptr)),
                sclearColor(std::exchange(sother.sclearColor, std::array<float, 4>{}))
            {}

            RenderPass& operator=(RenderPass&& sother) noexcept{
                if (this != &sother){
                    release();
                    sowner = std::exchange(sother.sowner, nullptr);
                    sdevice = std::exchange(sother.sdevice, nullptr);
                    srenderPass = std::exchange(sother.srenderPass, nullptr);
                    sclearColor = std::exchange(sother.sclearColor, std::array<float, 4>{});
                }
                return *this;
            }

            ~RenderPass(){ release(); }

            // Prototipe: render_pass.render([]{ ... }) memakai clear color
            // default; overload dengan array warna menerima RGBA float.
            template<typename Func>
            std::expected<void, Error> render(Func&& sfunc) const{
                return renderImpl(sclearColor, std::forward<Func>(sfunc));
            }

            template<typename Func>
            std::expected<void, Error> render(std::array<float, 4> sclearColor, Func&& sfunc) const{
                return renderImpl(sclearColor, std::forward<Func>(sfunc));
            }

            void setClearColor(std::array<float, 4> scolor) noexcept { sclearColor = scolor; }

            std::array<float, 4> clearColor() const noexcept { return sclearColor; }

            VkRenderPass handle() const noexcept { return srenderPass; }

            VkDevice deviceHandle() const noexcept { return sdevice; }

            bool empty() const noexcept { return srenderPass == nullptr; }

        private:
            friend class Device;

            RenderPass(const Device* sowner, VkDevice sdevice, VkRenderPass srenderPass) noexcept :
                sowner(sowner),
                sdevice(sdevice),
                srenderPass(srenderPass),
                sclearColor({0.3F, 0.3F, 0.3F, 1.0F})
            {}

            template<typename Func>
            std::expected<void, Error> renderImpl(const std::array<float, 4>& sclearColor,
                                                  Func&& sfunc) const{
                if (auto started = beginRender(sclearColor); !started){
                    return std::unexpected(started.error());
                }

                try{
                    std::forward<Func>(sfunc)();
                }catch(...){
                    endRender();
                    const renaru::Error serror = renaru::Error::RenderPassRecordingFailed;
                    if (sowner != nullptr){
                        sowner->noteError(serror);
                    }
                    return std::unexpected(serror);
                }

                endRender();
                if (sowner != nullptr && sowner->lastError() != Error::None){
                    return std::unexpected(sowner->lastError());
                }
                return {};
            }

            std::expected<void, Error> beginRender(const std::array<float, 4>& sclearColor) const{
                if (srenderPass == nullptr || sowner == nullptr){
                    return std::unexpected(renaru::Error::RenderPassRecordingFailed);
                }

                const VkCommandBuffer scommandBuffer = sowner->activeCommandBuffer();
                const VkFramebuffer sframebuffer = sowner->framebufferForCurrentImage();
                const VkExtent2D sextent = sowner->frameExtent();
                if (scommandBuffer == nullptr || sframebuffer == nullptr ||
                    sextent.width == 0 || sextent.height == 0){
                    return std::unexpected(renaru::Error::RenderPassRecordingFailed);
                }

                VkClearValue sclear{};
                sclear.color.float32[0] = sclearColor[0];
                sclear.color.float32[1] = sclearColor[1];
                sclear.color.float32[2] = sclearColor[2];
                sclear.color.float32[3] = sclearColor[3];

                VkRenderPassBeginInfo sinfo{};
                sinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                sinfo.renderPass = srenderPass;
                sinfo.framebuffer = sframebuffer;
                sinfo.renderArea.offset = {0, 0};
                sinfo.renderArea.extent = sextent;
                sinfo.clearValueCount = 1;
                sinfo.pClearValues = &sclear;

                PFN_vkCmdBeginRenderPass pfnCmdBeginRenderPass =
                    reinterpret_cast<PFN_vkCmdBeginRenderPass>(
                        entry().resolveDevice(sdevice, "vkCmdBeginRenderPass"));
                if (pfnCmdBeginRenderPass == nullptr){
                    return std::unexpected(renaru::Error::RenderPassRecordingFailed);
                }
                pfnCmdBeginRenderPass(scommandBuffer, &sinfo, VK_SUBPASS_CONTENTS_INLINE);
                return {};
            }

            void endRender() const noexcept{
                if (sowner == nullptr){
                    return;
                }
                const VkCommandBuffer scommandBuffer = sowner->activeCommandBuffer();
                if (scommandBuffer == nullptr){
                    return;
                }
                PFN_vkCmdEndRenderPass pfnCmdEndRenderPass =
                    reinterpret_cast<PFN_vkCmdEndRenderPass>(
                        entry().resolveDevice(sdevice, "vkCmdEndRenderPass"));
                if (pfnCmdEndRenderPass == nullptr){
                    sowner->noteError(renaru::Error::RenderPassRecordingFailed);
                    return;
                }
                pfnCmdEndRenderPass(scommandBuffer);
            }

            void release() noexcept{
                if (srenderPass != nullptr && sdevice != nullptr){
                    PFN_vkDestroyRenderPass pfnDestroyRenderPass =
                        reinterpret_cast<PFN_vkDestroyRenderPass>(
                            entry().resolveDevice(sdevice, "vkDestroyRenderPass"));
                    if (pfnDestroyRenderPass != nullptr){
                        pfnDestroyRenderPass(sdevice, srenderPass, nullptr);
                    }
                    srenderPass = nullptr;
                }
                sowner = nullptr;
                sdevice = nullptr;
            }

            const Device* sowner = nullptr;
            VkDevice sdevice = nullptr;
            VkRenderPass srenderPass = nullptr;
            std::array<float, 4> sclearColor{};
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// RenderPass membutuhkan tipe lengkap Swapchain/RenderPass dari partisi ini.
std::expected<renaru::RenderPass, renaru::Error>
renaru::Device::createRenderPass(const renaru::Swapchain& sswapchain) const{
    if (sdevice == nullptr || sswapchain.empty()){
        return std::unexpected(renaru::Error::RenderPassCreationFailed);
    }

    VkAttachmentDescription scolorAttachment{};
    scolorAttachment.format = sswapchain.format();
    scolorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    scolorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    scolorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    scolorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    scolorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    scolorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    scolorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference scolorReference{};
    scolorReference.attachment = 0;
    scolorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription ssubpass{};
    ssubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    ssubpass.colorAttachmentCount = 1;
    ssubpass.pColorAttachments = &scolorReference;

    VkSubpassDependency sdependency{};
    sdependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    sdependency.dstSubpass = 0;
    sdependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sdependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sdependency.srcAccessMask = 0;
    sdependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    sinfo.attachmentCount = 1;
    sinfo.pAttachments = &scolorAttachment;
    sinfo.subpassCount = 1;
    sinfo.pSubpasses = &ssubpass;
    sinfo.dependencyCount = 1;
    sinfo.pDependencies = &sdependency;

    PFN_vkCreateRenderPass pfnCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(
        entry().resolveDevice(sdevice, "vkCreateRenderPass"));
    if (pfnCreateRenderPass == nullptr){
        return std::unexpected(renaru::Error::RenderPassCreationFailed);
    }

    VkRenderPass screated = nullptr;
    if (pfnCreateRenderPass(sdevice, &sinfo, nullptr, &screated) != VK_SUCCESS ||
        screated == nullptr){
        return std::unexpected(renaru::Error::RenderPassCreationFailed);
    }

    std::println("Aru RHI: RenderPass(created!!)");

    return RenderPass(this, sdevice, screated);
}
