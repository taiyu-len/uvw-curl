#ifndef UVW_CURL_GLOBAL_HPP
#define UVW_CURL_GLOBAL_HPP
#include "create_lock.hpp"
#include <memory>

namespace uvw_curl
{
struct Global
	: public CreateLock<Global>
	, public std::enable_shared_from_this<Global>
{
	Global(Key, long flags) noexcept;
	Global(Key) noexcept;
	~Global() noexcept;

	bool init() const noexcept;
	bool initialized;
};
} // namespace uvw_curl
#endif // UVW_CURL_GLOBAL_HPP
