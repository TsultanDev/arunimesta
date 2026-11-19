#pragma once

// Header ini untuk Translation Unit lain di aru_rhi: mereka melihat dispatcher
// sebagai `extern`. Definisi storage-nya hanya ada di src/instance.cppm.
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_DEFAULT_DISPATCHER_HANDLED

namespace vk::detail {
    class DispatchLoaderDynamic;
    extern DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

#include <vulkan/vulkan.hpp>
