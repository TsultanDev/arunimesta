module;

#include <Windows.h>
#include <vulkan/vulkan.h>

#include <expected>
#include <utility>

export module renaru:entry;

import :error;

export namespace renaru {
    class Instance;

    class Entry {
        private:
            static constexpr const char* sLibraryName = "vulkan-1.dll";

            HMODULE sModule = nullptr;
            PFN_vkGetInstanceProcAddr sGetInstanceProcAddr = nullptr;
            PFN_vkGetDeviceProcAddr sGetDeviceProcAddr = nullptr;

            Entry() noexcept{
                sModule = LoadLibraryA(sLibraryName);
                if (sModule == nullptr){
                    return;
                }
                sGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                    GetProcAddress(sModule, "vkGetInstanceProcAddr"));
                sGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
                    GetProcAddress(sModule, "vkGetDeviceProcAddr"));
                if (sGetInstanceProcAddr == nullptr || sGetDeviceProcAddr == nullptr){
                    sDestroy();
                }
            }

            void sDestroy() noexcept{
                if (sModule != nullptr){
                    FreeLibrary(sModule);
                    sModule = nullptr;
                }
                sGetInstanceProcAddr = nullptr;
                sGetDeviceProcAddr = nullptr;
            }

            // Hanya class renaru yang boleh memakai dispatcher Vulkan.
            friend class Instance;
            friend class Surface;
            friend class Adapter;
            friend class Device;
            friend class Queue;
            friend class Frame;
            friend class Resource;
            friend class Swapchain;
            friend class RenderPass;
            friend class Framebuffer;
            friend class CommandPool;
            friend class CommandBuffer;

            // Dispatch panas disimpan; command dingin diload lokal lewat resolver ini.
            using Proc = PFN_vkVoidFunction;

            Proc resolveGlobal(const char* sPName) const noexcept{
                if (sModule == nullptr){
                    return nullptr;
                }
                return reinterpret_cast<Proc>(GetProcAddress(sModule, sPName));
            }

            Proc resolveInstance(VkInstance sInstance, const char* sPName) const noexcept{
                if (sGetInstanceProcAddr == nullptr){
                    return nullptr;
                }
                return sGetInstanceProcAddr(sInstance, sPName);
            }

            Proc resolveDevice(VkDevice sDevice, const char* sPName) const noexcept{
                if (sGetDeviceProcAddr == nullptr){
                    return nullptr;
                }
                return sGetDeviceProcAddr(sDevice, sPName);
            }

        public:
            static std::expected<Entry, Error> init() noexcept{
                Entry sEntry;
                if (sEntry.sModule == nullptr ||
                    sEntry.sGetInstanceProcAddr == nullptr ||
                    sEntry.sGetDeviceProcAddr == nullptr){
                    return std::unexpected(Error::eDynamicLoaderFailed);
                }
                return sEntry;
            }

            ~Entry(){ sDestroy(); }

            Entry(const Entry&) = delete;
            Entry& operator=(const Entry&) = delete;

            Entry(Entry&& sOther) noexcept :
                sModule(std::exchange(sOther.sModule, nullptr)),
                sGetInstanceProcAddr(std::exchange(sOther.sGetInstanceProcAddr, nullptr)),
                sGetDeviceProcAddr(std::exchange(sOther.sGetDeviceProcAddr, nullptr))
            {}

            Entry& operator=(Entry&& sOther) noexcept{
                if (this != &sOther){
                    sDestroy();
                    sModule = std::exchange(sOther.sModule, nullptr);
                    sGetInstanceProcAddr = std::exchange(sOther.sGetInstanceProcAddr, nullptr);
                    sGetDeviceProcAddr = std::exchange(sOther.sGetDeviceProcAddr, nullptr);
                }
                return *this;
            }

            // Didefinisikan di partisi :instance karena butuh tipe Instance lengkap.
            std::expected<Instance, Error> createInstance();
    };
}
