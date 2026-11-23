set(MODULES
  "src/lib.cppm"
  "src/error.cppm"
  "src/instance.cppm"
  "src/surface.cppm"
  "src/adapter.cppm"
  "src/device.cppm"
  "src/frame.cppm"
  "src/queue.cppm"
  "src/resource.cppm"
  "src/swapchain.cppm"
  "src/render_pass.cppm"
  "src/framebuffer.cppm"
  "src/command_pool.cppm"
  "src/command_buffer.cppm"
)

find_package(Vulkan REQUIRED)

add_library(renaru STATIC)

target_include_directories(renaru PUBLIC "include")
target_link_libraries(renaru PUBLIC Vulkan::Headers winaru)

target_sources(
  renaru PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
