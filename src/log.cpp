#include "log.hpp"
namespace uvw_curl {
std::mutex log_mtx;
static size_t depth = 0;

std::ostream& log_depth(std::ostream& os)
{
	int i = 20;
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
}
