cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED ARUNIMESTA_VERSION OR ARUNIMESTA_VERSION STREQUAL "")
  set(ARUNIMESTA_VERSION "0.1.0")
endif()

project(arunimesta VERSION ${ARUNIMESTA_VERSION} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory("winaru")
add_subdirectory("renaru")
add_subdirectory("aru")
