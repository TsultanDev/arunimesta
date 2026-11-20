module;

#include <aru_rhi/vulkan.hpp>

#include <cstdint>
#include <memory>
#include <utility>

export module aru.rhi:resource;

import aru.standard;
import :device;

export namespace aru::rhi {
    enum class ResourceKind : std::uint8_t {
        None = 0,
        Image,
        Buffer,
    };

    class Resource;
}

export namespace aru::rhi {
    // Resource adalah tagged union:
    //   - Image  : { vk::Image image, vk::ImageView view }
    //   - Buffer : { vk::Buffer buffer, vk::BufferView view }
    // Image dari swapchain dipinjam; ImageView dimiliki. Buffer dan BufferView
    // keduanya dimiliki. Salinan tidak memiliki handle (mOwns = false), supaya
    // Resource tetap bisa dilewatkan oleh value tanpa double-destroy.
    class Resource {
        public:
            using ResultType = aru::standard::Result<Resource, Error>;

            Resource(const Resource& other);
            Resource(Resource&& other) noexcept;
            Resource& operator=(const Resource& other);
            Resource& operator=(Resource&& other) noexcept;
            ~Resource();

            Resource copy() const;
            Resource move() noexcept;

            ResourceKind kind() const noexcept;
            bool empty() const noexcept;
            vk::Device deviceHandle() const noexcept;

            vk::Image imageHandle() const noexcept;
            vk::ImageView imageViewHandle() const noexcept;
            vk::Buffer bufferHandle() const noexcept;
            vk::BufferView bufferViewHandle() const noexcept;

            vk::Extent2D extent() const noexcept;
            vk::DeviceSize bufferSize() const noexcept;

        private:
            friend class Device;

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

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Resource(vk::Device device, vk::Image image, vk::ImageView view,
                     vk::Extent2D extent, InitTag tag) noexcept;
            Resource(vk::Device device, vk::Buffer buffer, vk::BufferView view,
                     vk::DeviceSize size, InitTag tag) noexcept;
            Resource(const Resource& other, CopyTag tag) noexcept;
            Resource(Resource&& other, MoveTag tag) noexcept;

            void clearActive() noexcept;
            void reset() noexcept;
            void release() noexcept;

            Data mData;
            ResourceKind mKind = ResourceKind::None;
            vk::Device mDevice;
            vk::Extent2D mExtent;
            vk::DeviceSize mSize = 0;
            bool mOwns = false;
    };
}

aru::standard::Result<aru::rhi::Resource, aru::rhi::Error>
aru::rhi::Device::createImageResource(vk::Image image, vk::Format format, vk::Extent2D extent) const{
    if (this->empty() || static_cast<VkImage>(image) == nullptr){
        return aru::rhi::Error::ResourceCreationFailed;
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
        return aru::rhi::Error::ResourceCreationFailed;
    }

    return Resource(this->mDevice, image, view, extent, Resource::InitTag{});
}

aru::standard::Result<aru::rhi::Resource, aru::rhi::Error>
aru::rhi::Device::createBufferResource(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Format format) const{
    if (this->empty() || size == 0){
        return aru::rhi::Error::ResourceCreationFailed;
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
        return aru::rhi::Error::ResourceCreationFailed;
    }

    return Resource(this->mDevice, buffer, view, size, Resource::InitTag{});
}

aru::rhi::Resource::Resource(vk::Device device, vk::Image image, vk::ImageView view,
                             vk::Extent2D extent, InitTag) noexcept :
    mData(),
    mKind(ResourceKind::Image),
    mDevice(device),
    mExtent(extent),
    mSize(0),
    mOwns(true)
{
    std::construct_at(&this->mData.image, ImageData{image, view});
}

aru::rhi::Resource::Resource(vk::Device device, vk::Buffer buffer, vk::BufferView view,
                             vk::DeviceSize size, InitTag) noexcept :
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

aru::rhi::Resource::Resource(const Resource& other) :
    Resource(other, CopyTag{})
{}

aru::rhi::Resource::Resource(Resource&& other) noexcept :
    Resource(std::move(other), MoveTag{})
{}

aru::rhi::Resource::Resource(const Resource& other, CopyTag) noexcept :
    mData(),
    mKind(other.mKind),
    mDevice(other.mDevice),
    mExtent(other.mExtent),
    mSize(other.mSize),
    mOwns(false)
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
}

aru::rhi::Resource::Resource(Resource&& other, MoveTag) noexcept :
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

aru::rhi::Resource& aru::rhi::Resource::operator=(const Resource& other){
    if (this != &other){
        Resource fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::Resource& aru::rhi::Resource::operator=(Resource&& other) noexcept{
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

aru::rhi::Resource aru::rhi::Resource::copy() const{
    return Resource(*this, CopyTag{});
}

aru::rhi::Resource aru::rhi::Resource::move() noexcept{
    return Resource(std::move(*this), MoveTag{});
}

aru::rhi::ResourceKind aru::rhi::Resource::kind() const noexcept{
    return this->mKind;
}

bool aru::rhi::Resource::empty() const noexcept{
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

vk::Device aru::rhi::Resource::deviceHandle() const noexcept{
    return this->mDevice;
}

vk::Image aru::rhi::Resource::imageHandle() const noexcept{
    return this->mKind == ResourceKind::Image ? this->mData.image.image : vk::Image{};
}

vk::ImageView aru::rhi::Resource::imageViewHandle() const noexcept{
    return this->mKind == ResourceKind::Image ? this->mData.image.view : vk::ImageView{};
}

vk::Buffer aru::rhi::Resource::bufferHandle() const noexcept{
    return this->mKind == ResourceKind::Buffer ? this->mData.buffer.buffer : vk::Buffer{};
}

vk::BufferView aru::rhi::Resource::bufferViewHandle() const noexcept{
    return this->mKind == ResourceKind::Buffer ? this->mData.buffer.view : vk::BufferView{};
}

vk::Extent2D aru::rhi::Resource::extent() const noexcept{
    return this->mExtent;
}

vk::DeviceSize aru::rhi::Resource::bufferSize() const noexcept{
    return this->mSize;
}

void aru::rhi::Resource::clearActive() noexcept{
    if (this->mKind == ResourceKind::Buffer){
        std::destroy_at(&this->mData.buffer);
    }else{
        std::destroy_at(&this->mData.image);
    }
    std::construct_at(&this->mData.image, ImageData{});
}

void aru::rhi::Resource::reset() noexcept{
    this->clearActive();
    this->mKind = ResourceKind::None;
    this->mDevice = vk::Device{};
    this->mExtent = vk::Extent2D{};
    this->mSize = 0;
    this->mOwns = false;
}

void aru::rhi::Resource::release() noexcept{
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

aru::rhi::Resource::~Resource(){
    this->release();
}
