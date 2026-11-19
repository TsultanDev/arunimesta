set(SOURCES
  "src/lib.cppm"
  "src/array.cppm"
  "src/print.cppm"
  "src/result.cppm"
  "src/string.cppm"
  "src/vector.cppm"
)
add_library(aru_std STATIC)

target_sources(
  aru_std PUBLIC FILE_SET CXX_MODULES FILES ${SOURCES}
)
