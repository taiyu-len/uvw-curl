#include "log.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
namespace uvw_curl {
std::mutex log_mtx;
static size_t depth = 0;

std::ostream& log_depth(std::ostream& os)
{
	const int WIDTH = 10;
	using clock_t = std::chrono::system_clock;
	// print current time
	auto now = clock_t::now();
	auto now_c = clock_t::to_time_t(now);
	os << std::put_time(std::localtime(&now_c), "%H:%M:%S.");

	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	os << std::setw(4) << (ms % 1000);

	int i = WIDTH;
	while (--i > depth) {
		os << ' ';
	}
	while (--i >= 0) {
		os << "│";
	}
	return os << ' ';
}

logger_t::logger_t(std::mutex &x) noexcept
: lock(x) {}

logger_t::~logger_t() noexcept
{
	std::cerr << '\n';
}

tracer_t::tracer_t() {
	++depth;
}

tracer_t::~tracer_t()
{
	--depth;
}
} // namespace uvw_curl
