module;

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan.h>

#include <utility>

export module renaru:entry;

export namespace renaru {
    // Pemilik vulkan-1.dll dan dispatcher global. Entry tidak bisa disalin;
    // move dipakai bila objek dipindahkan, tetapi accessor global memakai
    // satu instance statis.
    class Entry {
        public:
            using Proc = PFN_vkVoidFunction;

            Entry(const Entry&) = delete;
            Entry& operator=(const Entry&) = delete;

            Entry(Entry&& sother) noexcept :
                smodule(std::exchange(sother.smodule, nullptr)),
                sgetInstanceProcAddr(std::exchange(sother.sgetInstanceProcAddr, nullptr)),
                sgetDeviceProcAddr(std::exchange(sother.sgetDeviceProcAddr, nullptr))
            {}

            Entry& operator=(Entry&& sother) noexcept{
                if (this != &sother){
                    release();
                    smodule = std::exchange(sother.smodule, nullptr);
                    sgetInstanceProcAddr = std::exchange(sother.sgetInstanceProcAddr, nullptr);
                    sgetDeviceProcAddr = std::exchange(sother.sgetDeviceProcAddr, nullptr);
                }
                return *this;
            }

            ~Entry(){ release(); }

            static Entry& shared() noexcept{
                static Entry sentry;
                return sentry;
            }

            bool valid() const noexcept{
                return smodule != nullptr &&
                       sgetInstanceProcAddr != nullptr &&
                       sgetDeviceProcAddr != nullptr;
            }

            // Dispatch jarang dipakai diload lokal lewat resolver di bawah.
            // Dua dispatcher panas disimpan sebagai field karena dipakai
            // hampir setiap resolusi command.
            Proc resolveGlobal(const char* spName) const noexcept{
                if (smodule == nullptr){
                    return nullptr;
                }
                return reinterpret_cast<Proc>(GetProcAddress(smodule, spName));
            }

            Proc resolveInstance(VkInstance sinstance, const char* spName) const noexcept{
                if (sgetInstanceProcAddr == nullptr){
                    return nullptr;
                }
                return sgetInstanceProcAddr(sinstance, spName);
            }

            Proc resolveDevice(VkDevice sdevice, const char* spName) const noexcept{
                if (sgetDeviceProcAddr == nullptr){
                    return nullptr;
                }
                return sgetDeviceProcAddr(sdevice, spName);
            }

        private:
            static constexpr const char* sLibraryName = "vulkan-1.dll";

            Entry() noexcept{
                smodule = LoadLibraryA(sLibraryName);
                if (smodule == nullptr){
                    return;
                }
                sgetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                    GetProcAddress(smodule, "vkGetInstanceProcAddr"));
                sgetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
                    GetProcAddress(smodule, "vkGetDeviceProcAddr"));
                if (sgetInstanceProcAddr == nullptr || sgetDeviceProcAddr == nullptr){
                    release();
                }
            }

            void release() noexcept{
                if (smodule != nullptr){
                    FreeLibrary(smodule);
                    smodule = nullptr;
                }
                sgetInstanceProcAddr = nullptr;
                sgetDeviceProcAddr = nullptr;
            }

            HMODULE smodule = nullptr;
            PFN_vkGetInstanceProcAddr sgetInstanceProcAddr = nullptr;
            PFN_vkGetDeviceProcAddr sgetDeviceProcAddr = nullptr;
    };

    // Accessor process-wide untuk semua partisi renaru.
    inline Entry& entry() noexcept{
        return Entry::shared();
    }
}
