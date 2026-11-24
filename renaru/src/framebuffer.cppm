module;

#include <renaru/vulkan.h>

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
        public:
            using ResultType = std::expected<Framebuffer, Error>;

            // VkFramebuffer adalah raw handle: salinan tidak diizinkan.
            Framebuffer(const Framebuffer&) = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;

            Framebuffer(Framebuffer&& sother) noexcept :
                sdevice(std::exchange(sother.sdevice, nullptr)),
                sframebuffer(std::exchange(sother.sframebuffer, nullptr)),
                swidth(std::exchange(sother.swidth, std::uint32_t{0})),
                sheight(std::exchange(sother.sheight, std::uint32_t{0}))
            {}

            Framebuffer& operator=(Framebuffer&& sother) noexcept{
                if (this != &sother){
                    release();
                    sdevice = std::exchange(sother.sdevice, nullptr);
                    sframebuffer = std::exchange(sother.sframebuffer, nullptr);
                    swidth = std::exchange(sother.swidth, std::uint32_t{0});
                    sheight = std::exchange(sother.sheight, std::uint32_t{0});
                }
                return *this;
            }

            ~Framebuffer(){ release(); }

            VkFramebuffer handle() const noexcept { return sframebuffer; }

            VkDevice deviceHandle() const noexcept { return sdevice; }

            std::uint32_t width() const noexcept { return swidth; }

            std::uint32_t height() const noexcept { return sheight; }

            bool empty() const noexcept { return sframebuffer == nullptr; }

        private:
            friend class Device;

            Framebuffer(VkDevice sdevice, VkFramebuffer sframebuffer,
                        std::uint32_t swidth, std::uint32_t sheight) noexcept :
                sdevice(sdevice),
                sframebuffer(sframebuffer),
                swidth(swidth),
                sheight(sheight)
            {}

            void release() noexcept{
                if (sframebuffer != nullptr && sdevice != nullptr){
                    PFN_vkDestroyFramebuffer pfnDestroyFramebuffer =
                        reinterpret_cast<PFN_vkDestroyFramebuffer>(
                            entry().resolveDevice(sdevice, "vkDestroyFramebuffer"));
                    if (pfnDestroyFramebuffer != nullptr){
                        pfnDestroyFramebuffer(sdevice, sframebuffer, nullptr);
                    }
                    sframebuffer = nullptr;
                }
                sdevice = nullptr;
                swidth = 0;
                sheight = 0;
            }

            VkDevice sdevice = nullptr;
            VkFramebuffer sframebuffer = nullptr;
            std::uint32_t swidth = 0;
            std::uint32_t sheight = 0;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// Framebuffer membutuhkan tipe lengkap RenderPass/Resource/Framebuffer di sini.
std::expected<renaru::Framebuffer, renaru::Error>
renaru::Device::createFramebuffer(const renaru::RenderPass& srenderPass,
                                  const renaru::Resource& simage) const{
    if (sdevice == nullptr || srenderPass.empty() || simage.empty()){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }
    if (simage.kind() != renaru::ResourceKind::Image){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    const VkImageView sattachment = simage.imageViewHandle();
    if (sattachment == nullptr){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    const VkExtent2D sextent = simage.extent();
    if (sextent.width == 0 || sextent.height == 0){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    VkFramebufferCreateInfo sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    sinfo.renderPass = srenderPass.handle();
    sinfo.attachmentCount = 1;
    sinfo.pAttachments = &sattachment;
    sinfo.width = sextent.width;
    sinfo.height = sextent.height;
    sinfo.layers = 1;

    PFN_vkCreateFramebuffer pfnCreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(
        entry().resolveDevice(sdevice, "vkCreateFramebuffer"));
    if (pfnCreateFramebuffer == nullptr){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    VkFramebuffer screated = nullptr;
    if (pfnCreateFramebuffer(sdevice, &sinfo, nullptr, &screated) != VK_SUCCESS ||
        screated == nullptr){
        return std::unexpected(renaru::Error::FramebufferCreationFailed);
    }

    registerFramebuffer(screated);

    return Framebuffer(sdevice, screated, sextent.width, sextent.height);
}
