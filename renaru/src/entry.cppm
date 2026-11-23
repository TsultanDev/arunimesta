module;
#include <vulkan/vulkan.h>
#include <cstdint>
export module renaru:entry;

static const char* LIB_NAME = "vulkan-1.dll";

export namespace renaru{
    class Entry{
        public:

        private:
            class Global{
                PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
                PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = nullptr;
                //......
            } global = {};
    };
}
