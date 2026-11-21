module;

#include <aru_rhi/vulkan.hpp>

#include <expected>
#include <vector>
#include <print>

#include <cstdint>
#include <iterator>
#include <utility>

export module aru.rhi:device;

import :instance;
import :adapter;

export namespace aru::rhi {
    class Resource;
    class Swapchain;
    class RenderPass;
    class Framebuffer;
    class Device;
}

export namespace aru::rhi {
    class Device {
        public:
            using ResultType = std::expected<Device, Error>;

            // Didefinisikan di partisi lain (butuh tipe lengkap masing-masing).
            std::expected<Queue, Error> requestQueue(QueueFlags flags) const;
            std::expected<Resource, Error> createImageResource(
                vk::Image image, vk::Format format, vk::Extent2D extent) const;
            std::expected<Resource, Error> createBufferResource(
                vk::DeviceSize size, vk::BufferUsageFlags usage, vk::Format format) const;
            std::expected<Swapchain, Error> createSwapchain(
                std::uint32_t width, std::uint32_t height) const;
            std::expected<std::vector<Resource>, Error> createSwapchainImages(
                const Swapchain& swapchain) const;
            std::expected<RenderPass, Error> createRenderPass(
                const Swapchain& swapchain) const;
            std::expected<Framebuffer, Error> createFramebuffer(
                const RenderPass& renderPass, const Resource& image) const;

            Device(const Device& other);
            Device(Device&& other) noexcept;
            Device& operator=(const Device& other);
            Device& operator=(Device&& other) noexcept;
            ~Device();

            Device copy() const;
            Device move() noexcept;

            vk::Device handle() const noexcept;
            vk::PhysicalDevice physicalDeviceHandle() const noexcept;
            vk::Instance instanceHandle() const noexcept;
            vk::SurfaceKHR surfaceHandle() const noexcept;
            std::uint32_t queueFamilyIndex() const noexcept;
            bool empty() const noexcept;

        private:
            friend class Adapter;

            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Device(vk::Instance instance, vk::PhysicalDevice physicalDevice,
                   vk::SurfaceKHR surface, vk::Device device,
                   std::uint32_t queueFamilyIndex, InitTag tag) noexcept;
            Device(const Device& other, CopyTag tag);
            Device(Device&& other, MoveTag tag) noexcept;

            void release() noexcept;

            vk::Instance mInstance;
            vk::PhysicalDevice mPhysicalDevice;
            vk::SurfaceKHR mSurface;
            vk::Device mDevice;
            std::uint32_t mQueueFamily;
    };
}

std::expected<aru::rhi::Device, aru::rhi::Error> aru::rhi::Adapter::createDevice(const aru::rhi::Adapter& adapter) const{
    if (this->empty() || adapter.empty()){
        return std::unexpected(aru::rhi::Error::DeviceCreationFailed);
    }

    const float priority = 1.0F;
    vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.setQueueFamilyIndex(adapter.mQueueFamily);
    queueInfo.setQueueCount(1);
    queueInfo.setPQueuePriorities(&priority);

    const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    vk::DeviceCreateInfo createInfo;
    createInfo.setQueueCreateInfoCount(1);
    createInfo.setPQueueCreateInfos(&queueInfo);
    createInfo.setEnabledExtensionCount(static_cast<std::uint32_t>(std::size(extensions)));
    createInfo.setPpEnabledExtensionNames(extensions);

    vk::Device created;
    try{
        created = adapter.mPhysicalDevice.createDevice(createInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(aru::rhi::Error::DeviceCreationFailed);
    }

    std::println("Aru RHI: Device(created!!)");

    return Device(adapter.mInstance, adapter.mPhysicalDevice, adapter.mSurface,
                  created, adapter.mQueueFamily, Device::InitTag{});
}

aru::rhi::Device::Device(vk::Instance instance, vk::PhysicalDevice physicalDevice,
                         vk::SurfaceKHR surface, vk::Device device,
                         std::uint32_t queueFamilyIndex, InitTag) noexcept :
    mInstance(instance),
    mPhysicalDevice(physicalDevice),
    mSurface(surface),
    mDevice(device),
    mQueueFamily(queueFamilyIndex)
{}

aru::rhi::Device::Device(const Device& other) :
    Device(other, CopyTag{})
{}

aru::rhi::Device::Device(Device&& other) noexcept :
    Device(std::move(other), MoveTag{})
{}

aru::rhi::Device::Device(const Device& other, CopyTag) :
    mInstance(other.mInstance),
    mPhysicalDevice(other.mPhysicalDevice),
    mSurface(other.mSurface),
    mDevice(),
    mQueueFamily(other.mQueueFamily)
{}

aru::rhi::Device::Device(Device&& other, MoveTag) noexcept :
    mInstance(other.mInstance),
    mPhysicalDevice(std::exchange(other.mPhysicalDevice, vk::PhysicalDevice{})),
    mSurface(std::exchange(other.mSurface, vk::SurfaceKHR{})),
    mDevice(std::exchange(other.mDevice, vk::Device{})),
    mQueueFamily(std::exchange(other.mQueueFamily, std::uint32_t{0}))
{}

aru::rhi::Device& aru::rhi::Device::operator=(const Device& other){
    if (this != &other){
        Device fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::Device& aru::rhi::Device::operator=(Device&& other) noexcept{
    if (this != &other){
        this->release();
        this->mInstance = other.mInstance;
        this->mPhysicalDevice = std::exchange(other.mPhysicalDevice, vk::PhysicalDevice{});
        this->mSurface = std::exchange(other.mSurface, vk::SurfaceKHR{});
        this->mDevice = std::exchange(other.mDevice, vk::Device{});
        this->mQueueFamily = std::exchange(other.mQueueFamily, std::uint32_t{0});
    }
    return *this;
}

aru::rhi::Device aru::rhi::Device::copy() const{
    return Device(*this, CopyTag{});
}

aru::rhi::Device aru::rhi::Device::move() noexcept{
    return Device(std::move(*this), MoveTag{});
}

vk::Device aru::rhi::Device::handle() const noexcept{
    return this->mDevice;
}

vk::PhysicalDevice aru::rhi::Device::physicalDeviceHandle() const noexcept{
    return this->mPhysicalDevice;
}

vk::Instance aru::rhi::Device::instanceHandle() const noexcept{
    return this->mInstance;
}

vk::SurfaceKHR aru::rhi::Device::surfaceHandle() const noexcept{
    return this->mSurface;
}

std::uint32_t aru::rhi::Device::queueFamilyIndex() const noexcept{
    return this->mQueueFamily;
}

bool aru::rhi::Device::empty() const noexcept{
    return static_cast<VkDevice>(this->mDevice) == nullptr;
}

void aru::rhi::Device::release() noexcept{
    if (static_cast<VkDevice>(this->mDevice) != nullptr){
        this->mDevice.destroy(nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        this->mDevice = vk::Device{};
    }
    this->mPhysicalDevice = vk::PhysicalDevice{};
    this->mSurface = vk::SurfaceKHR{};
    this->mQueueFamily = 0;
}

aru::rhi::Device::~Device(){
    this->release();
}
