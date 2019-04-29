add_executable(uc_test)
target_link_libraries(uc_test PRIVATE uvw_curl)
target_sources(uc_test PRIVATE
	main.cpp
)
