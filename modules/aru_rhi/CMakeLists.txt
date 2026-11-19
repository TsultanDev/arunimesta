set(MODULES
  "src/lib.cppm"
  "src/instance.cppm"
)

find_package(Vulkan REQUIRED)

add_library(aru_rhi STATIC)

target_include_directories(aru_rhi PUBLIC "include")
target_link_libraries(aru_rhi PUBLIC Vulkan::Headers aru_std)

target_sources(
  aru_rhi PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
