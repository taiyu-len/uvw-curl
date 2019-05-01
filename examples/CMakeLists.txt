add_executable(print_headers print_headers.cpp)
target_link_libraries(print_headers PRIVATE uvw_curl)

add_executable(print_debug print_debug.cpp)
target_link_libraries(print_debug PRIVATE uvw_curl)

add_executable(image_scraper image_scraper.cpp)
target_link_libraries(image_scraper PRIVATE uvw_curl)
