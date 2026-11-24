module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <utility>

export module renaru:instance;

import winaru;
import :entry;
export import :error;

export namespace renaru {
    // Operasi bitwise constexpr supaya kombinasi flag bisa jadi konstanta compile-time.
    enum class QueueFlags : std::uint32_t {
        None = 0,
        Graphics = 1u << 0,
        Compute = 1u << 1,
        Transfer = 1u << 2,
        Present = 1u << 3,
    };

    constexpr QueueFlags operator|(QueueFlags sLhs, QueueFlags sRhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(sLhs) | static_cast<std::uint32_t>(sRhs));
    }

    constexpr QueueFlags operator&(QueueFlags sLhs, QueueFlags sRhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(sLhs) & static_cast<std::uint32_t>(sRhs));
    }

    constexpr QueueFlags operator^(QueueFlags sLhs, QueueFlags sRhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(sLhs) ^ static_cast<std::uint32_t>(sRhs));
    }

    constexpr QueueFlags operator~(QueueFlags sValue) noexcept{
        return static_cast<QueueFlags>(~static_cast<std::uint32_t>(sValue));
    }

    constexpr QueueFlags& operator|=(QueueFlags& sLhs, QueueFlags sRhs) noexcept{
        sLhs = sLhs | sRhs;
        return sLhs;
    }

    constexpr bool hasFlag(QueueFlags sValue, QueueFlags sFlag) noexcept{
        return (static_cast<std::uint32_t>(sValue) & static_cast<std::uint32_t>(sFlag)) != 0;
    }

    class Surface;
    class Adapter;
}

export namespace renaru {
    class Instance {
        private:
            using ResultType = std::expected<Instance, Error>;

            VkInstance sInstance = nullptr;
            std::string sApplicationName{};
            std::uint32_t sApiVersion = VK_API_VERSION_1_0;

            Instance(VkInstance sInstance, std::string&& sName,
                     std::uint32_t sVersion) noexcept :
                sInstance(sInstance),
                sApplicationName(std::move(sName)),
                sApiVersion(sVersion)
            {}

            static ResultType sCreate(const std::string& sAppName){
                if (!entry().valid()){
                    return std::unexpected(renaru::Error::DynamicLoaderFailed);
                }

                std::uint32_t sAvailableVersion = VK_API_VERSION_1_0;
                PFN_vkEnumerateInstanceVersion pfnEnumerateInstanceVersion =
                    reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
                        entry().resolveGlobal("vkEnumerateInstanceVersion"));
                if (pfnEnumerateInstanceVersion != nullptr){
                    if (pfnEnumerateInstanceVersion(&sAvailableVersion) != VK_SUCCESS){
                        return std::unexpected(renaru::Error::ApiVersionQueryFailed);
                    }
                }

                VkApplicationInfo sAppInfo{};
                sAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                sAppInfo.pApplicationName = sAppName.c_str();
                sAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
                sAppInfo.pEngineName = "Arunimesta";
                sAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
                sAppInfo.apiVersion = sAvailableVersion;

                const char* sExtensions[] = {
                    VK_KHR_SURFACE_EXTENSION_NAME,
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                };

                VkInstanceCreateInfo sCreateInfo{};
                sCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                sCreateInfo.pApplicationInfo = &sAppInfo;
                sCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(sExtensions));
                sCreateInfo.ppEnabledExtensionNames = sExtensions;

                PFN_vkCreateInstance pfnCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
                    entry().resolveGlobal("vkCreateInstance"));
                if (pfnCreateInstance == nullptr){
                    return std::unexpected(renaru::Error::DynamicLoaderFailed);
                }

                VkInstance sCreated = nullptr;
                if (pfnCreateInstance(&sCreateInfo, nullptr, &sCreated) != VK_SUCCESS ||
                    sCreated == nullptr){
                    return std::unexpected(renaru::Error::InstanceCreationFailed);
                }

                std::println("Aru RHI: Instance(created!!)");

                return Instance(sCreated, std::string(sAppName), sAvailableVersion);
            }

            void sDestroy() noexcept{
                if (sInstance != nullptr){
                    PFN_vkDestroyInstance pfnDestroyInstance =
                        reinterpret_cast<PFN_vkDestroyInstance>(
                            entry().resolveInstance(sInstance, "vkDestroyInstance"));
                    if (pfnDestroyInstance != nullptr){
                        pfnDestroyInstance(sInstance, nullptr);
                    }
                    sInstance = nullptr;
                }
                sApplicationName.clear();
                sApiVersion = VK_API_VERSION_1_0;
            }

        public:
            // sAppName adalah UTF-8; string_view tidak perlu null-terminated.
            static ResultType create(std::string_view sAppName){
                return sCreate(std::string(sAppName));
            }

            ~Instance(){ sDestroy(); }

            Instance(const Instance&) = delete;
            Instance& operator=(const Instance&) = delete;

            Instance(Instance&& sOther) noexcept :
                sInstance(std::exchange(sOther.sInstance, nullptr)),
                sApplicationName(std::move(sOther.sApplicationName)),
                sApiVersion(std::exchange(sOther.sApiVersion, std::uint32_t{VK_API_VERSION_1_0}))
            {}

            Instance& operator=(Instance&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sInstance = std::exchange(sOther.sInstance, nullptr);
                    sApplicationName = std::move(sOther.sApplicationName);
                    sApiVersion = std::exchange(sOther.sApiVersion, std::uint32_t{VK_API_VERSION_1_0});
                }
                return *this;
            }

            std::expected<Surface, Error> createSurface(const winaru::Window& sWindow);
            std::expected<Adapter, Error> requestAdapter(const Surface& sSurface);
    };
}
