set(MODULES
  "src/lib.cppm"
  "src/error.cppm"
  "src/window.cppm"
)
add_library(winaru STATIC)
target_compile_definitions(winaru PRIVATE UNICODE _UNICODE)
target_link_libraries(winaru PRIVATE user32 gdi32)

target_sources(
  winaru PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
