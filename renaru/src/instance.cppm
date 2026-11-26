module;

#include <vulkan/vulkan.h>

#include <cstdint>
#include <expected>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
            Entry& sEntry;
            VkInstance sInstance{nullptr};
            VkDebugUtilsMessengerEXT sDebugMessenger{nullptr};
            std::uint32_t sApiVersion{VK_API_VERSION_1_0};

            Instance(Entry& sEntry, VkInstance sInstance,
                     VkDebugUtilsMessengerEXT sDebugMessenger,
                     std::uint32_t sVersion) noexcept :
                sEntry(sEntry),
                sInstance(sInstance),
                sDebugMessenger(sDebugMessenger),
                sApiVersion(sVersion)
            {}

            void destroy() noexcept{
                if (sInstance != nullptr){
                    if (sDebugMessenger != nullptr){
                        PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyMessenger =
                            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                                sEntry.resolveInstance(sInstance,
                                                       "vkDestroyDebugUtilsMessengerEXT"));
                        if (pfnDestroyMessenger != nullptr){
                            pfnDestroyMessenger(sInstance, sDebugMessenger, nullptr);
                        }
                        sDebugMessenger = nullptr;
                    }
                    PFN_vkDestroyInstance pfnDestroyInstance =
                        reinterpret_cast<PFN_vkDestroyInstance>(
                            sEntry.resolveInstance(sInstance, "vkDestroyInstance"));
                    if (pfnDestroyInstance != nullptr){
                        pfnDestroyInstance(sInstance, nullptr);
                    }
                    sInstance = nullptr;
                }
                sApiVersion = VK_API_VERSION_1_0;
            }

            // Data hanya bisa diakses sesama class renaru.
            friend class Entry;
            friend class Surface;
            friend class Adapter;

        public:
            ~Instance(){ destroy(); }

            Instance(const Instance&) = delete;
            Instance& operator=(const Instance&) = delete;

            Instance(Instance&& sOther) noexcept :
                sEntry(sOther.sEntry),
                sInstance(std::exchange(sOther.sInstance, nullptr)),
                sDebugMessenger(std::exchange(sOther.sDebugMessenger, nullptr)),
                sApiVersion(std::exchange(sOther.sApiVersion, std::uint32_t{VK_API_VERSION_1_0}))
            {}

            Instance& operator=(Instance&& sOther) noexcept{
                if (this != &sOther){
                    destroy();
                    // sEntry tidak di-assign: objek Entry tetap yang dipakai instance ini.
                    sInstance = std::exchange(sOther.sInstance, nullptr);
                    sDebugMessenger = std::exchange(sOther.sDebugMessenger, nullptr);
                    sApiVersion = std::exchange(sOther.sApiVersion, std::uint32_t{VK_API_VERSION_1_0});
                }
                return *this;
            }

            std::expected<Surface, Error> createSurface(const winaru::Window& sWindow);
            std::expected<Adapter, Error> requestAdapter(const Surface& sSurface);
    };
}

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT sSeverity,
        VkDebugUtilsMessageTypeFlagsEXT sTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* sData,
        void* sUserData){
    (void)sSeverity;
    (void)sTypes;
    (void)sUserData;
    std::println("Vulkan debug: {}", sData->pMessage);
    return VK_FALSE;
}
#endif

std::expected<renaru::Instance, renaru::Error>
renaru::Entry::createInstance(){
    if (sModule == nullptr ||
        sGetInstanceProcAddr == nullptr ||
        sGetDeviceProcAddr == nullptr){
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

    // Enumerasi dulu supaya hanya yang benar-benar tersedia yang diminta.
    PFN_vkEnumerateInstanceExtensionProperties pfnEnumerateExtensions =
        reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
            resolveGlobal("vkEnumerateInstanceExtensionProperties"));
    PFN_vkEnumerateInstanceLayerProperties pfnEnumerateLayers =
        reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
            resolveGlobal("vkEnumerateInstanceLayerProperties"));
    if (pfnEnumerateExtensions == nullptr || pfnEnumerateLayers == nullptr){
        return std::unexpected(Error::eDynamicLoaderFailed);
    }

    std::uint32_t sExtensionCount = 0;
    if (pfnEnumerateExtensions(nullptr, &sExtensionCount, nullptr) != VK_SUCCESS){
        return std::unexpected(Error::eApiVersionQueryFailed);
    }
    std::vector<VkExtensionProperties> sAvailableExtensions(sExtensionCount);
    if (pfnEnumerateExtensions(nullptr, &sExtensionCount, sAvailableExtensions.data()) != VK_SUCCESS){
        return std::unexpected(Error::eApiVersionQueryFailed);
    }

    std::uint32_t sLayerCount = 0;
    if (pfnEnumerateLayers(&sLayerCount, nullptr) != VK_SUCCESS){
        return std::unexpected(Error::eApiVersionQueryFailed);
    }
    std::vector<VkLayerProperties> sAvailableLayers(sLayerCount);
    if (pfnEnumerateLayers(&sLayerCount, sAvailableLayers.data()) != VK_SUCCESS){
        return std::unexpected(Error::eApiVersionQueryFailed);
    }

    std::vector<std::string> sExtensionStrings{};
    bool sHasSurface = false;
    bool sHasWin32Surface = false;
#ifndef NDEBUG
    bool sHasDebugUtils = false;
#endif
    for (const VkExtensionProperties& sProperties : sAvailableExtensions){
        const std::string_view sName(sProperties.extensionName);
        if (sName == VK_KHR_SURFACE_EXTENSION_NAME){
            sExtensionStrings.emplace_back(sProperties.extensionName);
            sHasSurface = true;
        }
        else if (sName == VK_KHR_WIN32_SURFACE_EXTENSION_NAME){
            sExtensionStrings.emplace_back(sProperties.extensionName);
            sHasWin32Surface = true;
        }
#ifndef NDEBUG
        else if (sName == VK_EXT_DEBUG_UTILS_EXTENSION_NAME){
            sExtensionStrings.emplace_back(sProperties.extensionName);
            sHasDebugUtils = true;
        }
#endif
    }
    if (!sHasSurface || !sHasWin32Surface){
        return std::unexpected(Error::eInstanceCreationFailed);
    }

    std::vector<std::string> sLayerStrings{};
#ifndef NDEBUG
    constexpr const char* sDebugLayerName = "VK_LAYER_KHRONOS_validation";
    for (const VkLayerProperties& sProperties : sAvailableLayers){
        if (std::string_view(sProperties.layerName) == sDebugLayerName){
            sLayerStrings.emplace_back(sProperties.layerName);
            break;
        }
    }
#endif

    std::vector<const char*> sExtensionNames{};
    sExtensionNames.reserve(sExtensionStrings.size());
    for (const std::string& sName : sExtensionStrings){
        sExtensionNames.push_back(sName.c_str());
    }

    std::vector<const char*> sLayerNames{};
    sLayerNames.reserve(sLayerStrings.size());
    for (const std::string& sName : sLayerStrings){
        sLayerNames.push_back(sName.c_str());
    }

    VkInstanceCreateInfo sCreateInfo{};
    sCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    sCreateInfo.pApplicationInfo = &sAppInfo;
    sCreateInfo.enabledLayerCount = static_cast<std::uint32_t>(sLayerNames.size());
    sCreateInfo.ppEnabledLayerNames = sLayerNames.data();
    sCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(sExtensionNames.size());
    sCreateInfo.ppEnabledExtensionNames = sExtensionNames.data();

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

    VkDebugUtilsMessengerEXT sDebugMessenger = nullptr;
#ifndef NDEBUG
    if (sHasDebugUtils){
        auto sDestroyCreated = [&]() noexcept {
            PFN_vkDestroyInstance pfnDestroy = reinterpret_cast<PFN_vkDestroyInstance>(
                resolveInstance(sCreated, "vkDestroyInstance"));
            if (pfnDestroy != nullptr){
                pfnDestroy(sCreated, nullptr);
            }
        };

        PFN_vkCreateDebugUtilsMessengerEXT pfnCreateMessenger =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                resolveInstance(sCreated, "vkCreateDebugUtilsMessengerEXT"));
        if (pfnCreateMessenger == nullptr){
            sDestroyCreated();
            return std::unexpected(Error::eDynamicLoaderFailed);
        }

        VkDebugUtilsMessengerCreateInfoEXT sMessengerInfo{};
        sMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        sMessengerInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        sMessengerInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        sMessengerInfo.pfnUserCallback = debugCallback;

        if (pfnCreateMessenger(sCreated, &sMessengerInfo, nullptr, &sDebugMessenger) != VK_SUCCESS){
            sDestroyCreated();
            return std::unexpected(Error::eInstanceCreationFailed);
        }
    }
#endif

    std::println("Aru RHI: Instance(created!!)");

    return Instance(*this, sCreated, sDebugMessenger, sAvailableVersion);
}
