if(NOT DEFINED RENARU_VERSION OR RENARU_VERSION STREQUAL "")
  set(RENARU_VERSION "0.1.0")
endif()

project(renaru VERSION ${RENARU_VERSION} LANGUAGES C CXX)

set(MODULES
  "src/lib.cppm"
  "src/entry.cppm"
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

target_compile_definitions(renaru PUBLIC
  VK_USE_PLATFORM_WIN32_KHR=1
  ARUNIMESTA_NAME=\"${CMAKE_PROJECT_NAME}\"
  RENARU_NAME=\"${PROJECT_NAME}\"
  ARUNIMESTA_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
  ARUNIMESTA_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
  ARUNIMESTA_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
  RENARU_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  RENARU_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  RENARU_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
target_link_libraries(renaru PUBLIC Vulkan::Headers winaru)

target_sources(
  renaru PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
