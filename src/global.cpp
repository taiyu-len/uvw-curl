#include "global.hpp"
#include <curl/curl.h>
namespace uvw_curl
{
Global::Global(Key k) noexcept : Global(k, CURL_GLOBAL_DEFAULT) {}

Global::Global(Key, long flags) noexcept
: std::enable_shared_from_this<Global>()
, initialized(curl_global_init(flags) == 0)
{}

Global::~Global() noexcept
{
	if (initialized)
	{
		curl_global_cleanup();
	}
}

bool Global::init() const noexcept
{
	return initialized;
}
} // namespace uvw_curl
