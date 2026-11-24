module;

#include <renaru/vulkan.h>

#include <cstdint>
#include <expected>
#include <memory>
#include <utility>

export module renaru:resource;

import :entry;
import :device;

export namespace renaru {
    enum class ResourceKind : std::uint8_t {
        None = 0,
        Image,
        Buffer,
    };

    // Resource adalah tagged union:
    //   - Image  : { VkImage simage, VkImageView sview }
    //   - Buffer : { VkBuffer sbuffer, VkBufferView sview }
    // Image dari swapchain dipinjam; ImageView dimiliki. Buffer dan BufferView
    // keduanya dimiliki. Resource bersifat move-only karena handle-nya raw.
    class Resource {
        private:
            struct ImageData {
                VkImage image;
                VkImageView view;
            };

            struct BufferData {
                VkBuffer buffer;
                VkBufferView view;
            };

            union Data {
                ImageData image;
                BufferData buffer;

                Data() noexcept : image{} {}
                ~Data() {}
            };

        public:
            using ResultType = std::expected<Resource, Error>;

            // Handle Vulkan raw: salinan tidak diizinkan.
            Resource(const Resource&) = delete;
            Resource& operator=(const Resource&) = delete;

            Resource(Resource&& sother) noexcept :
                sdata(),
                skind(sother.skind),
                sdevice(sother.sdevice),
                sextent(sother.sextent),
                ssize(sother.ssize),
                sowns(sother.sowns)
            {
                switch (sother.skind){
                    case ResourceKind::Image:
                        std::construct_at(&sdata.image, sother.sdata.image);
                        break;
                    case ResourceKind::Buffer:
                        std::destroy_at(&sdata.image);
                        std::construct_at(&sdata.buffer, sother.sdata.buffer);
                        break;
                    default:
                        break;
                }
                sother.reset();
            }

            Resource& operator=(Resource&& sother) noexcept{
                if (this != &sother){
                    release();

                    skind = sother.skind;
                    sdevice = sother.sdevice;
                    sextent = sother.sextent;
                    ssize = sother.ssize;
                    sowns = sother.sowns;

                    switch (sother.skind){
                        case ResourceKind::Image:
                            std::construct_at(&sdata.image, sother.sdata.image);
                            break;
                        case ResourceKind::Buffer:
                            std::destroy_at(&sdata.image);
                            std::construct_at(&sdata.buffer, sother.sdata.buffer);
                            break;
                        default:
                            break;
                    }
                    sother.reset();
                }
                return *this;
            }

            ~Resource(){ release(); }

            ResourceKind kind() const noexcept { return skind; }

            bool empty() const noexcept{
                switch (skind){
                    case ResourceKind::Image:
                        return sdata.image.image == nullptr &&
                               sdata.image.view == nullptr;
                    case ResourceKind::Buffer:
                        return sdata.buffer.buffer == nullptr;
                    default:
                        return true;
                }
            }

            VkDevice deviceHandle() const noexcept { return sdevice; }

            VkImage imageHandle() const noexcept {
                return skind == ResourceKind::Image ? sdata.image.image : nullptr;
            }

            VkImageView imageViewHandle() const noexcept {
                return skind == ResourceKind::Image ? sdata.image.view : nullptr;
            }

            VkBuffer bufferHandle() const noexcept {
                return skind == ResourceKind::Buffer ? sdata.buffer.buffer : nullptr;
            }

            VkBufferView bufferViewHandle() const noexcept {
                return skind == ResourceKind::Buffer ? sdata.buffer.view : nullptr;
            }

            VkExtent2D extent() const noexcept { return sextent; }

            VkDeviceSize bufferSize() const noexcept { return ssize; }

        private:
            friend class Device;

            Resource(VkDevice sdevice, VkImage simage, VkImageView sview,
                     VkExtent2D sextent) noexcept :
                sdata(),
                skind(ResourceKind::Image),
                sdevice(sdevice),
                sextent(sextent),
                ssize(0),
                sowns(true)
            {
                std::construct_at(&sdata.image, ImageData{simage, sview});
            }

            Resource(VkDevice sdevice, VkBuffer sbuffer, VkBufferView sview,
                     VkDeviceSize ssize) noexcept :
                sdata(),
                skind(ResourceKind::Buffer),
                sdevice(sdevice),
                sextent(),
                ssize(ssize),
                sowns(true)
            {
                std::destroy_at(&sdata.image);
                std::construct_at(&sdata.buffer, BufferData{sbuffer, sview});
            }

            void clearActive() noexcept{
                if (skind == ResourceKind::Buffer){
                    std::destroy_at(&sdata.buffer);
                }else{
                    std::destroy_at(&sdata.image);
                }
                std::construct_at(&sdata.image, ImageData{});
            }

            void reset() noexcept{
                clearActive();
                skind = ResourceKind::None;
                sdevice = nullptr;
                sextent = VkExtent2D{};
                ssize = 0;
                sowns = false;
            }

            void release() noexcept{
                if (sowns && sdevice != nullptr){
                    switch (skind){
                        case ResourceKind::Image:
                            if (sdata.image.view != nullptr){
                                PFN_vkDestroyImageView pfnDestroyImageView =
                                    reinterpret_cast<PFN_vkDestroyImageView>(
                                        entry().resolveDevice(sdevice, "vkDestroyImageView"));
                                if (pfnDestroyImageView != nullptr){
                                    pfnDestroyImageView(sdevice, sdata.image.view, nullptr);
                                }
                            }
                            break;
                        case ResourceKind::Buffer:
                            if (sdata.buffer.view != nullptr){
                                PFN_vkDestroyBufferView pfnDestroyBufferView =
                                    reinterpret_cast<PFN_vkDestroyBufferView>(
                                        entry().resolveDevice(sdevice, "vkDestroyBufferView"));
                                if (pfnDestroyBufferView != nullptr){
                                    pfnDestroyBufferView(sdevice, sdata.buffer.view, nullptr);
                                }
                            }
                            if (sdata.buffer.buffer != nullptr){
                                PFN_vkDestroyBuffer pfnDestroyBuffer =
                                    reinterpret_cast<PFN_vkDestroyBuffer>(
                                        entry().resolveDevice(sdevice, "vkDestroyBuffer"));
                                if (pfnDestroyBuffer != nullptr){
                                    pfnDestroyBuffer(sdevice, sdata.buffer.buffer, nullptr);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
                reset();
            }

            Data sdata;
            ResourceKind skind = ResourceKind::None;
            VkDevice sdevice = nullptr;
            VkExtent2D sextent{};
            VkDeviceSize ssize = 0;
            bool sowns = false;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// resource membutuhkan tipe lengkap Resource dari partisi ini.
std::expected<renaru::Resource, renaru::Error>
renaru::Device::createImageResource(VkImage simage, VkFormat sformat, VkExtent2D sextent) const{
    if (sdevice == nullptr || simage == nullptr){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    VkImageViewCreateInfo sinfo{};
    sinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    sinfo.image = simage;
    sinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    sinfo.format = sformat;
    sinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    sinfo.subresourceRange.baseMipLevel = 0;
    sinfo.subresourceRange.levelCount = 1;
    sinfo.subresourceRange.baseArrayLayer = 0;
    sinfo.subresourceRange.layerCount = 1;

    PFN_vkCreateImageView pfnCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(
        entry().resolveDevice(sdevice, "vkCreateImageView"));
    if (pfnCreateImageView == nullptr){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    VkImageView sview = nullptr;
    if (pfnCreateImageView(sdevice, &sinfo, nullptr, &sview) != VK_SUCCESS || sview == nullptr){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    return Resource(sdevice, simage, sview, sextent);
}

std::expected<renaru::Resource, renaru::Error>
renaru::Device::createBufferResource(VkDeviceSize ssize, VkBufferUsageFlags susage, VkFormat sformat) const{
    if (sdevice == nullptr || ssize == 0){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    VkBufferCreateInfo sbufferInfo{};
    sbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sbufferInfo.size = ssize;
    sbufferInfo.usage = susage;
    sbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    PFN_vkCreateBuffer pfnCreateBuffer = reinterpret_cast<PFN_vkCreateBuffer>(
        entry().resolveDevice(sdevice, "vkCreateBuffer"));
    PFN_vkCreateBufferView pfnCreateBufferView = reinterpret_cast<PFN_vkCreateBufferView>(
        entry().resolveDevice(sdevice, "vkCreateBufferView"));
    if (pfnCreateBuffer == nullptr || pfnCreateBufferView == nullptr){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    VkBuffer sbuffer = nullptr;
    if (pfnCreateBuffer(sdevice, &sbufferInfo, nullptr, &sbuffer) != VK_SUCCESS ||
        sbuffer == nullptr){
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    VkBufferViewCreateInfo sviewInfo{};
    sviewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    sviewInfo.buffer = sbuffer;
    sviewInfo.format = sformat;
    sviewInfo.offset = 0;
    sviewInfo.range = ssize;

    VkBufferView sview = nullptr;
    if (pfnCreateBufferView(sdevice, &sviewInfo, nullptr, &sview) != VK_SUCCESS ||
        sview == nullptr){
        PFN_vkDestroyBuffer pfnDestroyBuffer = reinterpret_cast<PFN_vkDestroyBuffer>(
            entry().resolveDevice(sdevice, "vkDestroyBuffer"));
        if (pfnDestroyBuffer != nullptr){
            pfnDestroyBuffer(sdevice, sbuffer, nullptr);
        }
        return std::unexpected(renaru::Error::ResourceCreationFailed);
    }

    return Resource(sdevice, sbuffer, sview, ssize);
}
