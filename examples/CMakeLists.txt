add_executable(print_headers print_headers.cpp)
target_link_libraries(print_headers PRIVATE uvw_curl)

add_executable(print_debug print_debug.cpp)
target_link_libraries(print_debug PRIVATE uvw_curl)
