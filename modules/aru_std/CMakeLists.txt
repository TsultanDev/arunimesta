set(SOURCES
  "src/lib.cppm"
)
add_library(aru_std STATIC)

target_sources(
  aru_std PUBLIC FILE_SET CXX_MODULES FILES ${SOURCES}
)
