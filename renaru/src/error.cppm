module;

#include <cstdint>

export module renaru:error;

export namespace renaru {
    enum class Error : std::uint32_t {
        eNone                           = 0x00000000,
        eDynamicLoaderFailed             = 0x00000001,
        eApiVersionQueryFailed           = 0x00000002,
        eInstanceCreationFailed          = 0x00000003,
        eSurfaceCreationFailed           = 0x00000004,
        eAdapterNotFound                 = 0x00000005,
        eDeviceCreationFailed            = 0x00000006,
        eQueueRequestFailed              = 0x00000007,
        eSwapchainCreationFailed         = 0x00000008,
        eResourceCreationFailed          = 0x00000009,
        eRenderPassCreationFailed        = 0x0000000A,
        eFramebufferCreationFailed       = 0x0000000B,
        eCommandPoolCreationFailed       = 0x0000000C,
        eCommandBufferAllocationFailed   = 0x0000000D,
        eFrameAcquisitionFailed          = 0x0000000E,
        eCommandBufferRecordingFailed    = 0x0000000F,
        eRenderPassRecordingFailed       = 0x00000010,
        eCommandSubmissionFailed         = 0x00000011,
        eFramePresentationFailed         = 0x00000012,
        eMaxEnum                         = 0xFFFFFFFF
    };
}
