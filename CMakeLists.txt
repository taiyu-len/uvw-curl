cmake_minimum_required(VERSION 3.14)
project(uvw_curl VERSION 0.1)
add_subdirectory(external) # handles external dependencies

string(APPEND CLANG_TIDY_CHECKS *) # all by default
string(APPEND CLANG_TIDY_CHECKS ,-readability-*)
string(APPEND CLANG_TIDY_CHECKS ,-fushia-*)
string(APPEND CLANG_TIDY_CHECKS ,-hicpp-signed-bitwise) # allow bitflag macro
string(APPEND CLANG_TIDY_CHECKS ,-hicpp-vararg)  # allow using c functions
string(APPEND CLANG_TIDY_CHECKS ,-cppcoreguidelines-pro-type-vararg) # same
string(APPEND CLANG_TIDY_CHECKS ,-hicpp-no-array-decay) # allow __file__ macro
string(APPEND CLANG_TIDY_CHECKS ,-cppcoreguidelines-pro-bounds-array-to-pointer-decay) # same
string(APPEND CLANG_TIDY_CHECKS ,-cppcoreguidelines-pro-bounds-pointer-arithmetic) # allow argv[i]

set(CMAKE_CXX_CLANG_TIDY clang-tidy "-checks=${CLANG_TIDY_CHECKS}")
add_library(uvw_curl STATIC)
add_subdirectory(src)  # handles library source files
add_subdirectory(test) # handles tests

target_link_libraries(uvw_curl PRIVATE Threads::Threads)
target_link_libraries(uvw_curl PUBLIC sanitize_address)
target_compile_options(uvw_curl PUBLIC -Wall)

# TODO
# add proper targets for these
target_link_libraries(uvw_curl PRIVATE uv)
target_link_libraries(uvw_curl PRIVATE curl)
