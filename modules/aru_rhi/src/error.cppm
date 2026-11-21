module;

#include <format>
#include <string_view>

export module aru.rhi:error;

export namespace aru::rhi {
    enum class Error {
        None = 0,
        DynamicLoaderFailed,
        ApiVersionQueryFailed,
        InstanceCreationFailed,
        SurfaceCreationFailed,
        AdapterNotFound,
        DeviceCreationFailed,
        QueueRequestFailed,
        SwapchainCreationFailed,
        ResourceCreationFailed,
        RenderPassCreationFailed,
        FramebufferCreationFailed,
        CommandPoolCreationFailed,
        CommandBufferAllocationFailed,
    };
}

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
                case aru::rhi::Error::SurfaceCreationFailed:
                    text = "vk::createWin32SurfaceKHR failed";
                    break;
                case aru::rhi::Error::AdapterNotFound:
                    text = "no suitable physical device";
                    break;
                case aru::rhi::Error::DeviceCreationFailed:
                    text = "vk::createDevice failed";
                    break;
                case aru::rhi::Error::QueueRequestFailed:
                    text = "requested queue flags are unavailable";
                    break;
                case aru::rhi::Error::SwapchainCreationFailed:
                    text = "vk::createSwapchainKHR failed";
                    break;
                case aru::rhi::Error::ResourceCreationFailed:
                    text = "image view / buffer creation failed";
                    break;
                case aru::rhi::Error::RenderPassCreationFailed:
                    text = "vk::createRenderPass failed";
                    break;
                case aru::rhi::Error::FramebufferCreationFailed:
                    text = "vk::createFramebuffer failed";
                    break;
                case aru::rhi::Error::CommandPoolCreationFailed:
                    text = "vk::createCommandPool failed";
                    break;
                case aru::rhi::Error::CommandBufferAllocationFailed:
                    text = "vk::allocateCommandBuffers failed";
                    break;
            }
            return formatter<string_view>::format(text, ctx);
        }
    };
}
