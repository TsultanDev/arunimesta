module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <utility>

export module renaru:framebuffer;

import :entry;
import :device;
import :resource;
import :render_pass;

export namespace renaru {
    class Framebuffer {
        private:
            using ResultType = std::expected<Framebuffer, Error>;

            VkDevice sDevice = nullptr;
            VkFramebuffer sFramebuffer = nullptr;
            std::uint32_t sWidth = 0;
            std::uint32_t sHeight = 0;

            Framebuffer(VkDevice sDevice, VkFramebuffer sFramebuffer,
                        std::uint32_t sWidth, std::uint32_t sHeight) noexcept :
                sDevice(sDevice),
                sFramebuffer(sFramebuffer),
                sWidth(sWidth),
                sHeight(sHeight)
            {}

            void sDestroy() noexcept{
                if (sFramebuffer != nullptr && sDevice != nullptr){
                    PFN_vkDestroyFramebuffer pfnDestroyFramebuffer =
                        reinterpret_cast<PFN_vkDestroyFramebuffer>(
                            entry().resolveDevice(sDevice, "vkDestroyFramebuffer"));
                    if (pfnDestroyFramebuffer != nullptr){
                        pfnDestroyFramebuffer(sDevice, sFramebuffer, nullptr);
                    }
                    sFramebuffer = nullptr;
                }
                sDevice = nullptr;
                sWidth = 0;
                sHeight = 0;
            }

            friend class Device;

        public:
            ~Framebuffer(){ sDestroy(); }

            Framebuffer(const Framebuffer&) = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;

            Framebuffer(Framebuffer&& sOther) noexcept :
                sDevice(std::exchange(sOther.sDevice, nullptr)),
                sFramebuffer(std::exchange(sOther.sFramebuffer, nullptr)),
                sWidth(std::exchange(sOther.sWidth, std::uint32_t{0})),
                sHeight(std::exchange(sOther.sHeight, std::uint32_t{0}))
            {}

            Framebuffer& operator=(Framebuffer&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sDevice = std::exchange(sOther.sDevice, nullptr);
                    sFramebuffer = std::exchange(sOther.sFramebuffer, nullptr);
                    sWidth = std::exchange(sOther.sWidth, std::uint32_t{0});
                    sHeight = std::exchange(sOther.sHeight, std::uint32_t{0});
                }
                return *this;
            }
    };
}

std::expected<renaru::Framebuffer, renaru::Error>
renaru::Device::createFramebuffer(const renaru::RenderPass& sRenderPass,
                                  const renaru::Resource& sImage) const{
    if (sDevice == nullptr || sRenderPass.sRenderPass == nullptr ||
        sImage.sKind != renaru::ResourceKind::Image){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    const VkImageView sAttachment = sImage.sData.sImage.sView;
    if (sAttachment == nullptr){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    const VkExtent2D sExtent = sImage.sExtent;
    if (sExtent.width == 0 || sExtent.height == 0){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    VkFramebufferCreateInfo sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    sInfo.renderPass = sRenderPass.sRenderPass;
    sInfo.attachmentCount = 1;
    sInfo.pAttachments = &sAttachment;
    sInfo.width = sExtent.width;
    sInfo.height = sExtent.height;
    sInfo.layers = 1;

    PFN_vkCreateFramebuffer pfnCreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(
        entry().resolveDevice(sDevice, "vkCreateFramebuffer"));
    if (pfnCreateFramebuffer == nullptr){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    VkFramebuffer sCreated = nullptr;
    if (pfnCreateFramebuffer(sDevice, &sInfo, nullptr, &sCreated) != VK_SUCCESS ||
        sCreated == nullptr){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    sRegisterFramebuffer(sCreated);

    return Framebuffer(sDevice, sCreated, sExtent.width, sExtent.height);
}
