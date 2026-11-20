#pragma once

// Header ini untuk Translation Unit lain di aru_rhi: mereka melihat dispatcher
// sebagai `extern`. Definisi storage-nya hanya ada di src/instance.cppm.
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_DEFAULT_DISPATCHER_HANDLED
// Win32 dipakai Surface. Harus didefinisikan di semua TU agar layout
// DispatchLoaderDynamic identik (storage-nya ada di instance.cppm).
#define VK_USE_PLATFORM_WIN32_KHR 1

namespace vk::detail {
    class DispatchLoaderDynamic;
    extern DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

#include <vulkan/vulkan.hpp>
