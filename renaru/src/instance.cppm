module;

#include <renaru/vulkan.h>

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
    // Dipakai Device::requestQueue(). Operator bitwise constexpr supaya
    // kombinasi seperti GRAPHICS | PRESENT bisa jadi konstanta compile-time.
    enum class QueueFlags : std::uint32_t {
        None = 0,
        Graphics = 1u << 0,
        Compute = 1u << 1,
        Transfer = 1u << 2,
        Present = 1u << 3,
    };

    constexpr QueueFlags operator|(QueueFlags slhs, QueueFlags srhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(slhs) | static_cast<std::uint32_t>(srhs));
    }

    constexpr QueueFlags operator&(QueueFlags slhs, QueueFlags srhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(slhs) & static_cast<std::uint32_t>(srhs));
    }

    constexpr QueueFlags operator^(QueueFlags slhs, QueueFlags srhs) noexcept{
        return static_cast<QueueFlags>(static_cast<std::uint32_t>(slhs) ^ static_cast<std::uint32_t>(srhs));
    }

    constexpr QueueFlags operator~(QueueFlags svalue) noexcept{
        return static_cast<QueueFlags>(~static_cast<std::uint32_t>(svalue));
    }

    constexpr QueueFlags& operator|=(QueueFlags& slhs, QueueFlags srhs) noexcept{
        slhs = slhs | srhs;
        return slhs;
    }

    constexpr bool hasFlag(QueueFlags svalue, QueueFlags sflag) noexcept{
        return (static_cast<std::uint32_t>(svalue) & static_cast<std::uint32_t>(sflag)) != 0;
    }

    // Forward declaration: tipe lengkap ada di partisi :surface / :adapter.
    class Surface;
    class Adapter;
}

export namespace renaru {
    class Instance {
        public:
            using ResultType = std::expected<Instance, Error>;

            // sappName adalah UTF-8; string_view tidak perlu null-terminated.
            static ResultType create(std::string_view sappName){
                return createImpl(std::string(sappName));
            }

            // Didefinisikan di :surface / :adapter (butuh tipe lengkap).
            std::expected<Surface, Error> createSurface(const winaru::Window& swindow);
            std::expected<Adapter, Error> requestAdapter(const Surface& ssurface);

            // VkInstance adalah raw handle: salinan tidak diizinkan.
            Instance(const Instance&) = delete;
            Instance& operator=(const Instance&) = delete;

            Instance(Instance&& sother) noexcept :
                sinstance(std::exchange(sother.sinstance, nullptr)),
                sapplicationName(std::move(sother.sapplicationName)),
                sapiVersion(std::exchange(sother.sapiVersion, std::uint32_t{VK_API_VERSION_1_0}))
            {}

            Instance& operator=(Instance&& sother) noexcept{
                if (this != &sother){
                    release();
                    sinstance = std::exchange(sother.sinstance, nullptr);
                    sapplicationName = std::move(sother.sapplicationName);
                    sapiVersion = std::exchange(sother.sapiVersion, std::uint32_t{VK_API_VERSION_1_0});
                }
                return *this;
            }

            ~Instance(){ release(); }

            VkInstance handle() const noexcept { return sinstance; }

            std::uint32_t apiVersion() const noexcept { return sapiVersion; }

            const std::string& applicationName() const noexcept { return sapplicationName; }

            bool empty() const noexcept { return sinstance == nullptr; }

        private:
            Instance(VkInstance sinstance, std::string&& sname,
                     std::uint32_t sversion) noexcept :
                sinstance(sinstance),
                sapplicationName(std::move(sname)),
                sapiVersion(sversion)
            {}

            static ResultType createImpl(const std::string& sappName){
                if (!entry().valid()){
                    return std::unexpected(renaru::Error::DynamicLoaderFailed);
                }

                // Versi API maksimum loader. Vulkan 1.0 tidak punya
                // vkEnumerateInstanceVersion, jadi fallback ke 1.0.
                std::uint32_t savailableVersion = VK_API_VERSION_1_0;
                PFN_vkEnumerateInstanceVersion pfnEnumerateInstanceVersion =
                    reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
                        entry().resolveGlobal("vkEnumerateInstanceVersion"));
                if (pfnEnumerateInstanceVersion != nullptr){
                    if (pfnEnumerateInstanceVersion(&savailableVersion) != VK_SUCCESS){
                        return std::unexpected(renaru::Error::ApiVersionQueryFailed);
                    }
                }

                VkApplicationInfo sappInfo{};
                sappInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                sappInfo.pApplicationName = sappName.c_str();
                sappInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
                sappInfo.pEngineName = "Arunimesta";
                sappInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
                sappInfo.apiVersion = savailableVersion;

                // Dibutuhkan Surface Win32. Tanpa ini createWin32SurfaceKHR gagal.
                const char* sextensions[] = {
                    VK_KHR_SURFACE_EXTENSION_NAME,
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                };

                VkInstanceCreateInfo screateInfo{};
                screateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                screateInfo.pApplicationInfo = &sappInfo;
                screateInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(sextensions));
                screateInfo.ppEnabledExtensionNames = sextensions;

                PFN_vkCreateInstance pfnCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
                    entry().resolveGlobal("vkCreateInstance"));
                if (pfnCreateInstance == nullptr){
                    return std::unexpected(renaru::Error::DynamicLoaderFailed);
                }

                VkInstance screated = nullptr;
                if (pfnCreateInstance(&screateInfo, nullptr, &screated) != VK_SUCCESS ||
                    screated == nullptr){
                    return std::unexpected(renaru::Error::InstanceCreationFailed);
                }

                std::println("Aru RHI: Instance(created!!)");

                return Instance(screated, std::string(sappName), savailableVersion);
            }

            void release() noexcept{
                if (sinstance != nullptr){
                    PFN_vkDestroyInstance pfnDestroyInstance =
                        reinterpret_cast<PFN_vkDestroyInstance>(
                            entry().resolveInstance(sinstance, "vkDestroyInstance"));
                    if (pfnDestroyInstance != nullptr){
                        pfnDestroyInstance(sinstance, nullptr);
                    }
                    sinstance = nullptr;
                }
                sapplicationName.clear();
                sapiVersion = VK_API_VERSION_1_0;
            }

            VkInstance sinstance = nullptr;
            std::string sapplicationName;
            std::uint32_t sapiVersion = VK_API_VERSION_1_0;
    };
}
