set(MODULES
  "src/lib.cppm"
)
add_library(aru_window STATIC)
target_include_directories(aru_window PUBLIC "include")
target_link_libraries(aru_window PRIVATE user32 gdi32)

target_sources(
  aru_window PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
