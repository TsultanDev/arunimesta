module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <iterator>
#include <print>
#include <string>
#include <utility>

export module renaru:instance;

import winaru;
import :entry;
export import :error;

export namespace renaru {
    class Surface;
    class Adapter;
}

export namespace renaru {
    class Instance {
        private:
            using ResultType = std::expected<Instance, Error>;

            Entry& sEntry;
            VkInstance sInstance{nullptr};
            std::string sApplicationName{};
            std::uint32_t sApiVersion{VK_API_VERSION_1_0};

            Instance(Entry& sEntry, VkInstance sInstance, std::string&& sName,
                     std::uint32_t sVersion) noexcept :
                sEntry(sEntry),
                sInstance(sInstance),
                sApplicationName(std::move(sName)),
                sApiVersion(sVersion)
            {}

            void destroy() noexcept{
                if (sInstance != nullptr){
                    PFN_vkDestroyInstance pfnDestroyInstance =
                        reinterpret_cast<PFN_vkDestroyInstance>(
                            sEntry.resolveInstance(sInstance, "vkDestroyInstance"));
                    if (pfnDestroyInstance != nullptr){
                        pfnDestroyInstance(sInstance, nullptr);
                    }
                    sInstance = nullptr;
                }
                sApplicationName.clear();
                sApiVersion = VK_API_VERSION_1_0;
            }

            friend class Entry;

        public:
            static ResultType create(){
                return entry().createInstance();
            }

            ~Instance(){ destroy(); }

            Instance(const Instance&) = delete;
            Instance& operator=(const Instance&) = delete;

            Instance(Instance&& sOther) noexcept :
                sEntry(sOther.sEntry),
                sInstance(std::exchange(sOther.sInstance, nullptr)),
                sApplicationName(std::move(sOther.sApplicationName)),
                sApiVersion(std::exchange(sOther.sApiVersion, std::uint32_t{VK_API_VERSION_1_0}))
            {}

            Instance& operator=(Instance&& sOther) noexcept{
                if (this != &sOther){
                    destroy();
                    // sEntry tidak di-assign: selalu merujuk Entry::shared().
                    sInstance = std::exchange(sOther.sInstance, nullptr);
                    sApplicationName = std::move(sOther.sApplicationName);
                    sApiVersion = std::exchange(sOther.sApiVersion, std::uint32_t{VK_API_VERSION_1_0});
                }
                return *this;
            }

            Entry& entryRef() const noexcept{
                return sEntry;
            }

            std::expected<Surface, Error> createSurface(const winaru::Window& sWindow);
            std::expected<Adapter, Error> requestAdapter(const Surface& sSurface);
    };
}

// Entry::createInstance() didefinisikan di sini karena butuh tipe lengkap Instance.
std::expected<renaru::Instance, renaru::Error>
renaru::Entry::createInstance(){
    if (!valid()){
        return std::unexpected(Error::eDynamicLoaderFailed);
    }

    std::uint32_t sAvailableVersion = VK_API_VERSION_1_0;
    PFN_vkEnumerateInstanceVersion pfnEnumerateInstanceVersion =
        reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
            resolveGlobal("vkEnumerateInstanceVersion"));
    if (pfnEnumerateInstanceVersion != nullptr){
        if (pfnEnumerateInstanceVersion(&sAvailableVersion) != VK_SUCCESS){
            return std::unexpected(Error::eApiVersionQueryFailed);
        }
    }

    VkApplicationInfo sAppInfo{};
    sAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    sAppInfo.pApplicationName = ARUNIMESTA_NAME;
    sAppInfo.applicationVersion = VK_MAKE_VERSION(
        ARUNIMESTA_VERSION_MAJOR,
        ARUNIMESTA_VERSION_MINOR,
        ARUNIMESTA_VERSION_PATCH);
    sAppInfo.pEngineName = RENARU_NAME;
    sAppInfo.engineVersion = VK_MAKE_VERSION(
        RENARU_VERSION_MAJOR,
        RENARU_VERSION_MINOR,
        RENARU_VERSION_PATCH);
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
        resolveGlobal("vkCreateInstance"));
    if (pfnCreateInstance == nullptr){
        return std::unexpected(Error::eDynamicLoaderFailed);
    }

    VkInstance sCreated = nullptr;
    if (pfnCreateInstance(&sCreateInfo, nullptr, &sCreated) != VK_SUCCESS ||
        sCreated == nullptr){
        return std::unexpected(Error::eInstanceCreationFailed);
    }

    std::println("Aru RHI: Instance(created!!)");

    return Instance(*this, sCreated, std::string(ARUNIMESTA_NAME), sAvailableVersion);
}
