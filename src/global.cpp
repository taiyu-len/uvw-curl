#include "global.hpp"
#include <curl/curl.h>
namespace uvw_curl
{
auto Global::create() -> std::shared_ptr<Global>
{
	return Global::create(CURL_GLOBAL_DEFAULT);
}

auto Global::create(long flags) -> std::shared_ptr<Global>
{
	return curl_global_init(flags)
		? nullptr
		: std::make_shared<Global>(Key{});
}

Global::Global(Key x) noexcept
: CreateLock<Global>(x)
, std::enable_shared_from_this<Global>()
{}

Global::~Global() noexcept
{
	curl_global_cleanup();
}
}
