cmake_minimum_required(VERSION 2.8.0)
project(c2ocaml)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY /common/plugins)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY /common/plugins)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY /common/plugins)

include(cotire)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/../*.cpp")

add_definitions(-O3 -std=c++14 -Wall -Wextra -Wpedantic -fPIC -fno-rtti -DREPO_ROOT="${CMAKE_SOURCE_DIR}")
add_library(c2ocaml SHARED ${SOURCES})

include_directories(c2ocaml /mnt/gcc7.3.0/lib/gcc/x86_64-linux-gnu/7.3.0/plugin/include)

target_link_libraries(c2ocaml gmp)
target_link_libraries(c2ocaml gmpxx)
target_link_libraries(c2ocaml stdc++fs)

set_target_properties(c2ocaml PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${CMAKE_SOURCE_DIR}/../Common/pch.hpp")
set_target_properties(c2ocaml PROPERTIES PREFIX  "")
set_target_properties(c2ocaml PROPERTIES SUFFIX  "")

cotire(c2ocaml)
