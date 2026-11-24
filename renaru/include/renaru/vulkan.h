#pragma once

// Semua TU renaru memakai Vulkan C API. Platform Win32 diaktifkan sebelum
// header Vulkan dimasukkan agar VkWin32SurfaceCreateInfoKHR tersedia.
#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan.h>
