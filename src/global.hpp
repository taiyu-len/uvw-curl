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
	static auto create(long flags) -> std::shared_ptr<Global>;
	static auto create()           -> std::shared_ptr<Global>;

	Global(Key) noexcept;
	~Global() noexcept;
};
}
#endif // UVW_CURL_GLOBAL_HPP
