set(MODULES
  "src/lib.cppm"
  "src/error.cppm"
  "src/instance.cppm"
  "src/surface.cppm"
  "src/adapter.cppm"
  "src/device.cppm"
  "src/queue.cppm"
  "src/resource.cppm"
  "src/swapchain.cppm"
  "src/render_pass.cppm"
  "src/framebuffer.cppm"
  "src/command_pool.cppm"
  "src/command_buffer.cppm"
)

find_package(Vulkan REQUIRED)

add_library(aru_rhi STATIC)

target_include_directories(aru_rhi PUBLIC "include")
target_link_libraries(aru_rhi PUBLIC Vulkan::Headers aru_window)

target_sources(
  aru_rhi PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
