set(MODULES
    "src/lib.cppm"
    "src/application.cppm"
)

add_executable(aru "src/main.cpp")
target_include_directories(aru PRIVATE "include")

target_link_libraries(aru PRIVATE aru_window)

target_sources(aru
    PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
