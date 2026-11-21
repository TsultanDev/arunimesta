module;

// Dynamic Dispatch: tidak menautkan vulkan-1.lib. vkGetInstanceProcAddr dan
// entry point dimuat lewat Vulkan-Hpp DynamicLoaderTool saat runtime.
// Header ini hanya memberi deklarasi extern; storage ada di bawah.
#include <aru_rhi/vulkan.hpp>

#include <cstdint>
#include <expected>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <utility>

// === Storage utama Vulkan-Hpp dispatcher (hanya di file ini) ===
namespace vk::detail {
    DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

export module aru.rhi:instance;

import aru.window;
export import :error;

export namespace aru::rhi {
    // Dipakai Device::requestQueue(). Operator bitwise constexpr supaya
    // kombinasi seperti GRAPHICS | PRESENT bisa jadi konstanta compile-time.
    enum class QueueFlags : std::uint32_t {
        None = 0,
        Graphics = 1u << 0,
        Compute = 1u << 1,
        Transfer = 1u << 2,
        Present = 1u << 3,

    };

    constexpr QueueFlags operator|(QueueFlags lhs, QueueFlags rhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
    }

    constexpr QueueFlags operator&(QueueFlags lhs, QueueFlags rhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
    }

    constexpr QueueFlags operator^(QueueFlags lhs, QueueFlags rhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(lhs) ^ static_cast<std::uint32_t>(rhs));
    }

    constexpr QueueFlags operator~(QueueFlags value) noexcept{
        return static_cast<QueueFlags>(~static_cast<std::uint32_t>(value));
    }

    constexpr QueueFlags& operator|=(QueueFlags& lhs, QueueFlags rhs) noexcept{
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr bool hasFlag(QueueFlags value, QueueFlags flag) noexcept{
        return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
    }

    class Instance;
    class Surface;
    class Adapter;
    class Device;
    class Queue;
}

export namespace aru::rhi {
    class Instance {
        public:
            using ResultType = std::expected<Instance, Error>;

            // appName adalah UTF-8; string_view tidak perlu null-terminated.
            static ResultType create(std::string_view appName);

            // Didefinisikan di :surface / :adapter (butuh tipe lengkap).
            std::expected<Surface, Error> createSurface(const aru::window::Window& window);
            std::expected<Adapter, Error> requestAdapter(const Surface& surface);

            Instance(const Instance& other);
            Instance(Instance&& other) noexcept;
            Instance& operator=(const Instance& other);
            Instance& operator=(Instance&& other) noexcept;
            ~Instance();

            Instance copy() const;
            Instance move() noexcept;

            vk::Instance handle() const noexcept;
            std::uint32_t apiVersion() const noexcept;
            const std::string& applicationName() const noexcept;
            bool empty() const noexcept;

        private:
            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Instance(const Instance& other, CopyTag tag);
            Instance(Instance&& other, MoveTag tag) noexcept;
            Instance(vk::Instance handle, std::string&& name,
                     std::uint32_t version, InitTag tag) noexcept;
            static ResultType createImpl(const std::string& appName);

            void release() noexcept;

            vk::Instance mInstance;
            std::string mApplicationName;
            std::uint32_t mApiVersion;
    };
}

aru::rhi::Instance::ResultType aru::rhi::Instance::create(std::string_view appName){
    return createImpl(std::string(appName));
}

aru::rhi::Instance::ResultType aru::rhi::Instance::createImpl(const std::string& appName){
    // Dynamic Dispatch: muat vkGetInstanceProcAddr + entry-point level loader.
    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr == nullptr){
        return std::unexpected(aru::rhi::Error::DynamicLoaderFailed);
    }

    // Versi API maksimum yang didukung driver loader. Vulkan 1.0 tidak punya
    // vkEnumerateInstanceVersion, jadi fallback ke VK_API_VERSION_1_0.
    std::uint32_t availableVersion = VK_API_VERSION_1_0;
    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion != nullptr){
        const VkResult result = VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion(&availableVersion);
        if (result != VK_SUCCESS){
            return std::unexpected(aru::rhi::Error::ApiVersionQueryFailed);
        }
    }

    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName(appName.c_str());
    appInfo.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
    appInfo.setPEngineName("Arunimesta");
    appInfo.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
    appInfo.setApiVersion(availableVersion);

    // Dibutuhkan Surface Win32. Tanpa ini createWin32SurfaceKHR gagal.
    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo(&appInfo);
    createInfo.setEnabledExtensionCount(static_cast<std::uint32_t>(std::size(extensions)));
    createInfo.setPpEnabledExtensionNames(extensions);

    vk::Instance created;
    try{
        created = vk::createInstance(createInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return std::unexpected(aru::rhi::Error::InstanceCreationFailed);
    }

    // Load instance-level entry points setelah instance tersedia.
    VULKAN_HPP_DEFAULT_DISPATCHER.init(created);

    std::println("Aru RHI: Instance(created!!)");

    return Instance(created, std::string(appName), availableVersion, InitTag{});
}

aru::rhi::Instance::Instance(const Instance& other) :
    Instance(other, CopyTag{})
{}

aru::rhi::Instance::Instance(Instance&& other) noexcept :
    Instance(std::move(other), MoveTag{})
{}

aru::rhi::Instance::Instance(vk::Instance handle, std::string&& name,
                             std::uint32_t version, InitTag) noexcept :
    mInstance(handle),
    mApplicationName(std::move(name)),
    mApiVersion(version)
{}

aru::rhi::Instance::Instance(const Instance& other, CopyTag) :
    mInstance(),
    mApplicationName(other.mApplicationName),
    mApiVersion(other.mApiVersion)
{}

aru::rhi::Instance::Instance(Instance&& other, MoveTag) noexcept :
    mInstance(std::exchange(other.mInstance, vk::Instance{})),
    mApplicationName(std::move(other.mApplicationName)),
    mApiVersion(std::exchange(other.mApiVersion, std::uint32_t{VK_API_VERSION_1_0}))
{}

aru::rhi::Instance& aru::rhi::Instance::operator=(const Instance& other){
    if (this != &other){
        Instance fresh(other, CopyTag{});
        *this = std::move(fresh);
    }
    return *this;
}

aru::rhi::Instance& aru::rhi::Instance::operator=(Instance&& other) noexcept{
    if (this != &other){
        this->release();
        this->mInstance = std::exchange(other.mInstance, vk::Instance{});
        this->mApplicationName = std::move(other.mApplicationName);
        this->mApiVersion = std::exchange(other.mApiVersion, std::uint32_t{VK_API_VERSION_1_0});
    }
    return *this;
}

aru::rhi::Instance aru::rhi::Instance::copy() const{
    return Instance(*this, CopyTag{});
}

aru::rhi::Instance aru::rhi::Instance::move() noexcept{
    return Instance(std::move(*this), MoveTag{});
}

vk::Instance aru::rhi::Instance::handle() const noexcept{
    return this->mInstance;
}

std::uint32_t aru::rhi::Instance::apiVersion() const noexcept{
    return this->mApiVersion;
}

const std::string& aru::rhi::Instance::applicationName() const noexcept{
    return this->mApplicationName;
}

bool aru::rhi::Instance::empty() const noexcept{
    return static_cast<VkInstance>(this->mInstance) == nullptr;
}

void aru::rhi::Instance::release() noexcept{
    if (static_cast<VkInstance>(this->mInstance) != nullptr){
        this->mInstance.destroy(nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
        this->mInstance = vk::Instance{};
    }
    this->mApiVersion = VK_API_VERSION_1_0;
}

aru::rhi::Instance::~Instance(){
    this->release();
}
