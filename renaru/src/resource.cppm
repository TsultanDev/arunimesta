module;

#include <vulkan/vulkan.h>

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

    class Resource {
        private:
            using ResultType = std::expected<Resource, Error>;

            struct ImageData {
                VkImage sImage = nullptr;
                VkImageView sView = nullptr;
            };

            struct BufferData {
                VkBuffer sBuffer = nullptr;
                VkBufferView sView = nullptr;
            };

            union Data {
                ImageData sImage{};
                BufferData sBuffer;

                Data() noexcept : sImage{} {}
                ~Data() {}
            };

            Data sData{};
            ResourceKind sKind = ResourceKind::None;
            VkDevice sDevice = nullptr;
            VkExtent2D sExtent{};
            VkDeviceSize sSize = 0;
            bool sOwns = false;

            Resource(VkDevice sDevice, VkImage sImage, VkImageView sView,
                     VkExtent2D sExtent) noexcept :
                sData(),
                sKind(ResourceKind::Image),
                sDevice(sDevice),
                sExtent(sExtent),
                sSize(0),
                sOwns(true)
            {
                std::construct_at(&sData.sImage, ImageData{sImage, sView});
            }

            Resource(VkDevice sDevice, VkBuffer sBuffer, VkBufferView sView,
                     VkDeviceSize sSize) noexcept :
                sData(),
                sKind(ResourceKind::Buffer),
                sDevice(sDevice),
                sExtent(),
                sSize(sSize),
                sOwns(true)
            {
                std::destroy_at(&sData.sImage);
                std::construct_at(&sData.sBuffer, BufferData{sBuffer, sView});
            }

            void sDestroy() noexcept{
                if (sOwns && sDevice != nullptr){
                    switch (sKind){
                        case ResourceKind::Image:
                            if (sData.sImage.sView != nullptr){
                                PFN_vkDestroyImageView pfnDestroyImageView =
                                    reinterpret_cast<PFN_vkDestroyImageView>(
                                        entry().resolveDevice(sDevice, "vkDestroyImageView"));
                                if (pfnDestroyImageView != nullptr){
                                    pfnDestroyImageView(sDevice, sData.sImage.sView, nullptr);
                                }
                            }
                            break;
                        case ResourceKind::Buffer:
                            if (sData.sBuffer.sView != nullptr){
                                PFN_vkDestroyBufferView pfnDestroyBufferView =
                                    reinterpret_cast<PFN_vkDestroyBufferView>(
                                        entry().resolveDevice(sDevice, "vkDestroyBufferView"));
                                if (pfnDestroyBufferView != nullptr){
                                    pfnDestroyBufferView(sDevice, sData.sBuffer.sView, nullptr);
                                }
                            }
                            if (sData.sBuffer.sBuffer != nullptr){
                                PFN_vkDestroyBuffer pfnDestroyBuffer =
                                    reinterpret_cast<PFN_vkDestroyBuffer>(
                                        entry().resolveDevice(sDevice, "vkDestroyBuffer"));
                                if (pfnDestroyBuffer != nullptr){
                                    pfnDestroyBuffer(sDevice, sData.sBuffer.sBuffer, nullptr);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
                sReset();
            }

            void sClearActive() noexcept{
                if (sKind == ResourceKind::Buffer){
                    std::destroy_at(&sData.sBuffer);
                }else{
                    std::destroy_at(&sData.sImage);
                }
                std::construct_at(&sData.sImage, ImageData{});
            }

            void sReset() noexcept{
                sClearActive();
                sKind = ResourceKind::None;
                sDevice = nullptr;
                sExtent = VkExtent2D{};
                sSize = 0;
                sOwns = false;
            }

            friend class Device;

        public:
            ~Resource(){ sDestroy(); }

            Resource(const Resource&) = delete;
            Resource& operator=(const Resource&) = delete;

            Resource(Resource&& sOther) noexcept :
                sData(),
                sKind(sOther.sKind),
                sDevice(sOther.sDevice),
                sExtent(sOther.sExtent),
                sSize(sOther.sSize),
                sOwns(sOther.sOwns)
            {
                switch (sOther.sKind){
                    case ResourceKind::Image:
                        std::construct_at(&sData.sImage, sOther.sData.sImage);
                        break;
                    case ResourceKind::Buffer:
                        std::destroy_at(&sData.sImage);
                        std::construct_at(&sData.sBuffer, sOther.sData.sBuffer);
                        break;
                    default:
                        break;
                }
                sOther.sReset();
            }

            Resource& operator=(Resource&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();

                    sKind = sOther.sKind;
                    sDevice = sOther.sDevice;
                    sExtent = sOther.sExtent;
                    sSize = sOther.sSize;
                    sOwns = sOther.sOwns;

                    switch (sOther.sKind){
                        case ResourceKind::Image:
                            std::construct_at(&sData.sImage, sOther.sData.sImage);
                            break;
                        case ResourceKind::Buffer:
                            std::destroy_at(&sData.sImage);
                            std::construct_at(&sData.sBuffer, sOther.sData.sBuffer);
                            break;
                        default:
                            break;
                    }
                    sOther.sReset();
                }
                return *this;
            }
    };
}

std::expected<renaru::Resource, renaru::Error>
renaru::Device::createImageResource(VkImage sImage, VkFormat sFormat, VkExtent2D sExtent) const{
    if (sDevice == nullptr || sImage == nullptr){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    VkImageViewCreateInfo sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    sInfo.image = sImage;
    sInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    sInfo.format = sFormat;
    sInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    sInfo.subresourceRange.baseMipLevel = 0;
    sInfo.subresourceRange.levelCount = 1;
    sInfo.subresourceRange.baseArrayLayer = 0;
    sInfo.subresourceRange.layerCount = 1;

    PFN_vkCreateImageView pfnCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(
        entry().resolveDevice(sDevice, "vkCreateImageView"));
    if (pfnCreateImageView == nullptr){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    VkImageView sView = nullptr;
    if (pfnCreateImageView(sDevice, &sInfo, nullptr, &sView) != VK_SUCCESS || sView == nullptr){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    return Resource(sDevice, sImage, sView, sExtent);
}

std::expected<renaru::Resource, renaru::Error>
renaru::Device::createBufferResource(VkDeviceSize sSize, VkBufferUsageFlags sUsage, VkFormat sFormat) const{
    if (sDevice == nullptr || sSize == 0){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    VkBufferCreateInfo sBufferInfo{};
    sBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sBufferInfo.size = sSize;
    sBufferInfo.usage = sUsage;
    sBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    PFN_vkCreateBuffer pfnCreateBuffer = reinterpret_cast<PFN_vkCreateBuffer>(
        entry().resolveDevice(sDevice, "vkCreateBuffer"));
    PFN_vkCreateBufferView pfnCreateBufferView = reinterpret_cast<PFN_vkCreateBufferView>(
        entry().resolveDevice(sDevice, "vkCreateBufferView"));
    if (pfnCreateBuffer == nullptr || pfnCreateBufferView == nullptr){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    VkBuffer sBuffer = nullptr;
    if (pfnCreateBuffer(sDevice, &sBufferInfo, nullptr, &sBuffer) != VK_SUCCESS ||
        sBuffer == nullptr){
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    VkBufferViewCreateInfo sViewInfo{};
    sViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    sViewInfo.buffer = sBuffer;
    sViewInfo.format = sFormat;
    sViewInfo.offset = 0;
    sViewInfo.range = sSize;

    VkBufferView sView = nullptr;
    if (pfnCreateBufferView(sDevice, &sViewInfo, nullptr, &sView) != VK_SUCCESS ||
        sView == nullptr){
        PFN_vkDestroyBuffer pfnDestroyBuffer = reinterpret_cast<PFN_vkDestroyBuffer>(
            entry().resolveDevice(sDevice, "vkDestroyBuffer"));
        if (pfnDestroyBuffer != nullptr){
            pfnDestroyBuffer(sDevice, sBuffer, nullptr);
        }
        return std::unexpected(renaru::Error::eResourceCreationFailed);
    }

    return Resource(sDevice, sBuffer, sView, sSize);
}
