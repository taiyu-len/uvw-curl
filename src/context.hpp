#ifndef UVW_CURL_CONTEXT_HPP
#define UVW_CURL_CONTEXT_HPP
/// Private Header
#include "multi.hpp"
#include "easy.hpp"
#include <curl/curl.h>
#include <uvw/poll.hpp>

namespace uvw_curl
{
struct Context
{
	Context(Multi& multi, curl_socket_t s) noexcept;
	~Context() noexcept;

	Multi&
		multi;
	curl_socket_t
		s;
	std::shared_ptr<uvw::PollHandle>
		poll;
};
} // uvw_curl
#endif // UVW_CURL_CONTEXT_HPP
