set(MODULES
  "src/lib.cppm"
  "src/window.cppm"
)
add_library(aru_window STATIC)
target_include_directories(aru_window PUBLIC "include")
target_compile_definitions(aru_window PRIVATE UNICODE _UNICODE)
target_link_libraries(aru_window PRIVATE user32 gdi32)

target_sources(
  aru_window PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
