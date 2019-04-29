cmake_minimum_required(VERSION 3.14)
project(uvw_curl VERSION 0.1)
add_subdirectory(external) # handles external dependencies

set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=*,-readability-*,-fuchsia-*)
add_library(uvw_curl STATIC)
add_subdirectory(src)  # handles library source files
add_subdirectory(test) # handles tests

target_link_libraries(uvw_curl PRIVATE Threads::Threads)
target_link_libraries(uvw_curl PUBLIC sanitize_address)

# TODO
# add proper targets for these
target_link_libraries(uvw_curl PRIVATE uv)
target_link_libraries(uvw_curl PRIVATE curl)
