target_sources(uvw_curl PRIVATE
	global.cpp
	multi.cpp
	easy.cpp
	context.cpp
	log.cpp
)
set_property(TARGET uvw_curl PROPERTY PUBLIC_HEADER
	global.hpp
	multi.hpp
	easy.hpp
	uvw_curl.hpp
	create_lock.hpp
	log.hpp
)
target_include_directories(uvw_curl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
