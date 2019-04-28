#ifndef UVW_CURL_LOG_HPP
#define UVW_CURL_LOG_HPP
#include <mutex>
#include <iostream>

#define LOG() \
	uvw_curl::logger_t(uvw_curl::log_mtx) << uvw_curl::log_depth
#define TRACE() \
	uvw_curl::tracer_t __TRACE_OBJ; \
	LOG() << __func__ << "(): "

namespace uvw_curl {
std::ostream& log_depth(std::ostream&);
extern std::mutex log_mtx;

struct logger_t {
	template<typename T>
	std::ostream& operator<<(T&& x) {
		return std::cerr << x;
	}

	logger_t(std::mutex &) noexcept;
	~logger_t() noexcept;

	std::lock_guard<std::mutex> lock;
};

struct tracer_t {
	tracer_t();
	~tracer_t();
};
}

#endif // UVW_CURL_LOG_HPP
