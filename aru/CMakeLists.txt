set(MODULES
    "src/lib.cppm"
)

add_executable(aru "src/main.cpp")

target_link_libraries(aru PRIVATE winaru renaru)

target_sources(aru
    PUBLIC FILE_SET CXX_MODULES FILES ${MODULES}
)
