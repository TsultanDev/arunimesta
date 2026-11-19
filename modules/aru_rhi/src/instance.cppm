module;

// Dynamic Dispatch: tidak menautkan vulkan-1.lib. vkGetInstanceProcAddr dan
// entry point dimuat lewat Vulkan-Hpp DynamicLoaderTool saat runtime.
// Header ini hanya memberi deklarasi extern; storage ada di bawah.
#include <aru_rhi/vulkan.hpp>

#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

// === Storage utama Vulkan-Hpp dispatcher (hanya di file ini) ===
namespace vk::detail {
    DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

export module aru.rhi:instance;

import aru.standard;

export namespace aru::rhi {
    enum class Error {
        None = 0,
        DynamicLoaderFailed,
        ApiVersionQueryFailed,
        InstanceCreationFailed,
    };

    class Instance;
}

// Biar println("{}", err) bekerja seperti di aru.window.
export namespace std {
    template<>
    struct formatter<aru::rhi::Error> : formatter<string_view> {
        template<typename FormatContext>
        auto format(const aru::rhi::Error& error, FormatContext& ctx) const {
            string_view text = "unknown";
            switch (error) {
                case aru::rhi::Error::None:
                    text = "none";
                    break;
                case aru::rhi::Error::DynamicLoaderFailed:
                    text = "vulkan dynamic loader failed";
                    break;
                case aru::rhi::Error::ApiVersionQueryFailed:
                    text = "vkEnumerateInstanceVersion failed";
                    break;
                case aru::rhi::Error::InstanceCreationFailed:
                    text = "vk::createInstance failed";
                    break;
            }
            return formatter<string_view>::format(text, ctx);
        }
    };
}

export namespace aru::rhi {
    class Instance {
        public:
            using ResultType = aru::standard::Result<Instance, Error>;

            // Factory: appName boleh String<char8_t>, String<char16_t>, atau
            // String<char32_t>. Versi API Vulkan diambil dari
            // vkEnumerateInstanceVersion, bukan dari argumen pemanggil.
            template<aru::standard::character T>
            static ResultType create(const aru::standard::String<T>& appName);

            // Rule of Five. Copy menyalin metadata (handle ikut dikosongkan),
            // move memindahkan handle Vulkan ke objek baru.
            Instance(const Instance& other);
            Instance(Instance&& other) noexcept;
            Instance& operator=(const Instance& other);
            Instance& operator=(Instance&& other) noexcept;
            ~Instance();

            Instance copy() const;
            Instance move() noexcept;

            vk::Instance handle() const noexcept;
            std::uint32_t apiVersion() const noexcept;
            const aru::standard::String<char8_t>& applicationName() const noexcept;
            bool empty() const noexcept;

        private:
            struct CopyTag {};
            struct MoveTag {};
            struct InitTag {};

            Instance(const Instance& other, CopyTag tag);
            Instance(Instance&& other, MoveTag tag) noexcept;
            Instance(vk::Instance handle, aru::standard::String<char8_t>&& name,
                     std::uint32_t version, InitTag tag) noexcept;
            static ResultType createImpl(const aru::standard::String<char8_t>& appName);

            void release() noexcept;

            vk::Instance mInstance;
            aru::standard::String<char8_t> mApplicationName;
            std::uint32_t mApiVersion;
    };
}

template<aru::standard::character T>
aru::rhi::Instance::ResultType aru::rhi::Instance::create(const aru::standard::String<T>& appName){
    auto utf8 = aru::standard::String<char8_t>::from(appName);
    return createImpl(utf8);
}

aru::rhi::Instance::ResultType aru::rhi::Instance::createImpl(const aru::standard::String<char8_t>& appName){
    // Dynamic Dispatch: muat vkGetInstanceProcAddr + entry-point level loader.
    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr == nullptr){
        return aru::rhi::Error::DynamicLoaderFailed;
    }

    // Versi API maksimum yang didukung driver loader. Vulkan 1.0 tidak punya
    // vkEnumerateInstanceVersion, jadi fallback ke VK_API_VERSION_1_0.
    std::uint32_t availableVersion = VK_API_VERSION_1_0;
    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion != nullptr){
        const VkResult result = VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion(&availableVersion);
        if (result != VK_SUCCESS){
            return aru::rhi::Error::ApiVersionQueryFailed;
        }
    }

    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName(reinterpret_cast<const char*>(appName.stringData()));
    appInfo.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
    appInfo.setPEngineName("Arunimesta");
    appInfo.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
    appInfo.setApiVersion(availableVersion);

    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo(&appInfo);

    vk::Instance created;
    try{
        created = vk::createInstance(createInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }catch(...){
        return aru::rhi::Error::InstanceCreationFailed;
    }

    // Load instance-level entry points setelah instance tersedia.
    VULKAN_HPP_DEFAULT_DISPATCHER.init(created);

    aru::standard::println("Aru RHI: Instance(created!!)");

    return Instance(created, aru::standard::String<char8_t>(appName), availableVersion, InitTag{});
}

aru::rhi::Instance::Instance(const Instance& other) :
    Instance(other, CopyTag{})
{}

aru::rhi::Instance::Instance(Instance&& other) noexcept :
    Instance(std::move(other), MoveTag{})
{}

aru::rhi::Instance::Instance(vk::Instance handle, aru::standard::String<char8_t>&& name,
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
    mApplicationName(other.mApplicationName.move()),
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
        this->mApplicationName = other.mApplicationName.move();
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

const aru::standard::String<char8_t>& aru::rhi::Instance::applicationName() const noexcept{
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
