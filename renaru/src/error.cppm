module;

#include <format>
#include <string_view>

export module renaru:error;

export namespace renaru {
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
        FrameAcquisitionFailed,
        CommandBufferRecordingFailed,
        RenderPassRecordingFailed,
        CommandSubmissionFailed,
        FramePresentationFailed,
    };
}

export namespace std {
    template<>
    struct formatter<renaru::Error> : formatter<string_view> {
        template<typename FormatContext>
        auto format(const renaru::Error& error, FormatContext& ctx) const {
            string_view text = "unknown";
            switch (error) {
                case renaru::Error::None:
                    text = "none";
                    break;
                case renaru::Error::DynamicLoaderFailed:
                    text = "vulkan dynamic loader failed";
                    break;
                case renaru::Error::ApiVersionQueryFailed:
                    text = "vkEnumerateInstanceVersion failed";
                    break;
                case renaru::Error::InstanceCreationFailed:
                    text = "vk::createInstance failed";
                    break;
                case renaru::Error::SurfaceCreationFailed:
                    text = "vk::createWin32SurfaceKHR failed";
                    break;
                case renaru::Error::AdapterNotFound:
                    text = "no suitable physical device";
                    break;
                case renaru::Error::DeviceCreationFailed:
                    text = "vk::createDevice failed";
                    break;
                case renaru::Error::QueueRequestFailed:
                    text = "requested queue flags are unavailable";
                    break;
                case renaru::Error::SwapchainCreationFailed:
                    text = "vk::createSwapchainKHR failed";
                    break;
                case renaru::Error::ResourceCreationFailed:
                    text = "image view / buffer creation failed";
                    break;
                case renaru::Error::RenderPassCreationFailed:
                    text = "vk::createRenderPass failed";
                    break;
                case renaru::Error::FramebufferCreationFailed:
                    text = "vk::createFramebuffer failed";
                    break;
                case renaru::Error::CommandPoolCreationFailed:
                    text = "vk::createCommandPool failed";
                    break;
                case renaru::Error::CommandBufferAllocationFailed:
                    text = "vk::allocateCommandBuffers failed";
                    break;
                case renaru::Error::FrameAcquisitionFailed:
                    text = "vk::acquireNextImageKHR failed";
                    break;
                case renaru::Error::CommandBufferRecordingFailed:
                    text = "command buffer recording failed";
                    break;
                case renaru::Error::RenderPassRecordingFailed:
                    text = "render pass recording failed";
                    break;
                case renaru::Error::CommandSubmissionFailed:
                    text = "vkQueueSubmit failed";
                    break;
                case renaru::Error::FramePresentationFailed:
                    text = "vkQueuePresentKHR failed";
                    break;
            }
            return formatter<string_view>::format(text, ctx);
        }
    };
}
