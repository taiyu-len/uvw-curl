#include "easy.hpp"
#include "log.hpp"
#include "multi.hpp"

namespace uvw_curl
{

Easy::Easy(Key) noexcept
: uvw::Emitter<Easy>()
, std::enable_shared_from_this<Easy>()
, _handle(curl_easy_init(), &curl_easy_cleanup)
{
	curl_easy_setopt(_handle.get(), CURLOPT_WRITEFUNCTION, &write);
	curl_easy_setopt(_handle.get(), CURLOPT_XFERINFOFUNCTION, &xferinfo);
	curl_easy_setopt(_handle.get(), CURLOPT_WRITEDATA, this);
	curl_easy_setopt(_handle.get(), CURLOPT_XFERINFODATA, this);
}

void Easy::url(const char* x) noexcept
{
	curl_easy_setopt(_handle.get(), CURLOPT_URL, x);
}

auto Easy::url() noexcept -> const char*
{
	const char* x;
	curl_easy_getinfo(_handle.get(), CURLINFO_EFFECTIVE_URL, &x);
	return x;
}

size_t Easy::write(char* data, size_t size, size_t nmemb, Easy *easy) noexcept
{
	easy->publish(Easy::DataEvent{data, nmemb * size});
	return nmemb * size;
}

int Easy::xferinfo(
	Easy* easy,
	curl_off_t dltotal, curl_off_t dlnow,
	curl_off_t ultotal, curl_off_t ulnow) noexcept
{
	easy->publish(Easy::XferInfoEvent{dltotal, dlnow, ultotal, ulnow});
	return 0;
}

/**
 * emits EndEvent, and releases ownership of self.
 */
void Easy::finish() noexcept
{
	auto ptr = shared_from_this();
	_self.reset();
	publish(EndEvent{});
}
} // namespace uvw_curl
