set(MODULES
  "src/lib.cppm"
  "src/error.cppm"
  "src/window.cppm"
)
add_library(aru_window STATIC)
target_compile_definitions(aru_window PRIVATE UNICODE _UNICODE)
target_link_libraries(aru_window PRIVATE user32 gdi32 aru_std)

target_sources(
  aru_window PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
