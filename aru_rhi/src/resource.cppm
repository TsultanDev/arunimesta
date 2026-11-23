module;

#include <aru_rhi/vulkan.hpp>

#include <expected>

#include <cstdint>
#include <memory>
#include <utility>

export module aru.rhi:resource;

import :device;

export namespace aru::rhi {
    enum class ResourceKind : std::uint8_t {
        None = 0,
        Image,
        Buffer,
    };

    // Resource adalah tagged union:
    //   - Image  : { vk::Image image, vk::ImageView view }
    //   - Buffer : { vk::Buffer buffer, vk::BufferView view }
    // Image dari swapchain dipinjam; ImageView dimiliki. Buffer dan BufferView
    // keduanya dimiliki. Resource bersifat move-only karena handle-nya raw.
    class Resource {
        private:
            struct ImageData {
                vk::Image image;
                vk::ImageView view;
            };

            struct BufferData {
                vk::Buffer buffer;
                vk::BufferView view;
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

            Resource(Resource&& other) noexcept :
                mData(),
                mKind(other.mKind),
                mDevice(other.mDevice),
                mExtent(other.mExtent),
                mSize(other.mSize),
                mOwns(other.mOwns)
            {
                switch (other.mKind){
                    case ResourceKind::Image:
                        std::construct_at(&this->mData.image, other.mData.image);
                        break;
                    case ResourceKind::Buffer:
                        std::destroy_at(&this->mData.image);
                        std::construct_at(&this->mData.buffer, other.mData.buffer);
                        break;
                    default:
                        break;
                }
                other.reset();
            }

            Resource& operator=(Resource&& other) noexcept{
                if (this != &other){
                    this->release();

                    this->mKind = other.mKind;
                    this->mDevice = other.mDevice;
                    this->mExtent = other.mExtent;
                    this->mSize = other.mSize;
                    this->mOwns = other.mOwns;

                    switch (other.mKind){
                        case ResourceKind::Image:
                            std::construct_at(&this->mData.image, other.mData.image);
                            break;
                        case ResourceKind::Buffer:
                            std::destroy_at(&this->mData.image);
                            std::construct_at(&this->mData.buffer, other.mData.buffer);
                            break;
                        default:
                            break;
                    }
                    other.reset();
                }
                return *this;
            }

            ~Resource(){ this->release(); }

            ResourceKind kind() const noexcept { return this->mKind; }

            bool empty() const noexcept{
                switch (this->mKind){
                    case ResourceKind::Image:
                        return static_cast<VkImage>(this->mData.image.image) == nullptr &&
                               static_cast<VkImageView>(this->mData.image.view) == nullptr;
                    case ResourceKind::Buffer:
                        return static_cast<VkBuffer>(this->mData.buffer.buffer) == nullptr;
                    default:
                        return true;
                }
            }

            vk::Device deviceHandle() const noexcept { return this->mDevice; }

            vk::Image imageHandle() const noexcept {
                return this->mKind == ResourceKind::Image ? this->mData.image.image : vk::Image{};
            }

            vk::ImageView imageViewHandle() const noexcept {
                return this->mKind == ResourceKind::Image ? this->mData.image.view : vk::ImageView{};
            }

            vk::Buffer bufferHandle() const noexcept {
                return this->mKind == ResourceKind::Buffer ? this->mData.buffer.buffer : vk::Buffer{};
            }

            vk::BufferView bufferViewHandle() const noexcept {
                return this->mKind == ResourceKind::Buffer ? this->mData.buffer.view : vk::BufferView{};
            }

            vk::Extent2D extent() const noexcept { return this->mExtent; }

            vk::DeviceSize bufferSize() const noexcept { return this->mSize; }

        private:
            friend class Device;

            Resource(vk::Device device, vk::Image image, vk::ImageView view,
                     vk::Extent2D extent) noexcept :
                mData(),
                mKind(ResourceKind::Image),
                mDevice(device),
                mExtent(extent),
                mSize(0),
                mOwns(true)
            {
                std::construct_at(&this->mData.image, ImageData{image, view});
            }

            Resource(vk::Device device, vk::Buffer buffer, vk::BufferView view,
                     vk::DeviceSize size) noexcept :
                mData(),
                mKind(ResourceKind::Buffer),
                mDevice(device),
                mExtent(),
                mSize(size),
                mOwns(true)
            {
                std::destroy_at(&this->mData.image);
                std::construct_at(&this->mData.buffer, BufferData{buffer, view});
            }

            void clearActive() noexcept{
                if (this->mKind == ResourceKind::Buffer){
                    std::destroy_at(&this->mData.buffer);
                }else{
                    std::destroy_at(&this->mData.image);
                }
                std::construct_at(&this->mData.image, ImageData{});
            }

            void reset() noexcept{
                this->clearActive();
                this->mKind = ResourceKind::None;
                this->mDevice = vk::Device{};
                this->mExtent = vk::Extent2D{};
                this->mSize = 0;
                this->mOwns = false;
            }

            void release() noexcept{
                if (this->mOwns && static_cast<VkDevice>(this->mDevice) != nullptr){
                    switch (this->mKind){
                        case ResourceKind::Image:
                            if (static_cast<VkImageView>(this->mData.image.view) != nullptr){
                                this->mDevice.destroyImageView(this->mData.image.view, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
                            }
                            break;
                        case ResourceKind::Buffer:
                            if (static_cast<VkBufferView>(this->mData.buffer.view) != nullptr){
                                this->mDevice.destroyBufferView(this->mData.buffer.view, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
                            }
                            if (static_cast<VkBuffer>(this->mData.buffer.buffer) != nullptr){
                                this->mDevice.destroyBuffer(this->mData.buffer.buffer, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
                            }
                            break;
                        default:
                            break;
                    }
                }
                this->reset();
            }

            Data mData;
            ResourceKind mKind = ResourceKind::None;
            vk::Device mDevice;
            vk::Extent2D mExtent;
            vk::DeviceSize mSize = 0;
            bool mOwns = false;
    };
}

// Definisi lintas partisi: Device didefinisikan di :device, tetapi pembuatan
// resource membutuhkan tipe lengkap Resource dari partisi ini.
std::expected<aru::rhi::Resource, aru::rhi::Error>
aru::rhi::Device::createImageResource(vk::Image image, vk::Format format, vk::Extent2D extent) const{
    if (this->empty() || static_cast<VkImage>(image) == nullptr){
        return std::unexpected(aru::rhi::Error::ResourceCreationFailed);
    }

    vk::ImageViewCreateInfo info;
    info.setImage(image);
    info.setViewType(vk::ImageViewType::e2D);
    info.setFormat(format);

    vk::ImageSubresourceRange range;
    range.setAspectMask(vk::ImageAspectFlagBits::eColor);
    range.setBaseMipLevel(0);
    range.setLevelCount(1);
    range.setBaseArrayLayer(0);
    range.setLayerCount(1);
    info.setSubresourceRange(range);

    vk::ImageView view;
    try{
        view = this->mDevice.createImageView(info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(aru::rhi::Error::ResourceCreationFailed);
    }

    return Resource(this->mDevice, image, view, extent);
}

std::expected<aru::rhi::Resource, aru::rhi::Error>
aru::rhi::Device::createBufferResource(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Format format) const{
    if (this->empty() || size == 0){
        return std::unexpected(aru::rhi::Error::ResourceCreationFailed);
    }

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size);
    bufferInfo.setUsage(usage);
    bufferInfo.setSharingMode(vk::SharingMode::eExclusive);

    vk::Buffer buffer;
    vk::BufferView view;
    try{
        buffer = this->mDevice.createBuffer(bufferInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);

        vk::BufferViewCreateInfo viewInfo;
        viewInfo.setBuffer(buffer);
        viewInfo.setFormat(format);
        viewInfo.setOffset(0);
        viewInfo.setRange(size);
        view = this->mDevice.createBufferView(viewInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        if (static_cast<VkBuffer>(buffer) != nullptr){
            this->mDevice.destroyBuffer(buffer, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        }
        return std::unexpected(aru::rhi::Error::ResourceCreationFailed);
    }

    return Resource(this->mDevice, buffer, view, size);
}
