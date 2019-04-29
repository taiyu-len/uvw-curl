#ifndef UVW_CURL_MULTI_HPP
#define UVW_CURL_MULTI_HPP
#include "create_lock.hpp"

#include <curl/curl.h>
#include <memory>
#include <uvw/emitter.hpp>
#include <uvw/loop.hpp>
#include <uvw/timer.hpp>

namespace uvw_curl
{
struct Context;
struct Global;
struct Easy;
/**
 * Manages the CURLM multi handle for use with uvw
 */
struct Multi
	: public CreateLock<Multi>
	, public uvw::Emitter<Multi>
	, public std::enable_shared_from_this<Multi>
{
	struct ErrorEvent
	{
		CURLMcode code;
		const char* what() const noexcept { return curl_multi_strerror(code); }
	};

	static auto create(std::shared_ptr<uvw::Loop>, std::shared_ptr<Global>)
		-> std::shared_ptr<Multi>;

	Multi(Key) noexcept;
	~Multi() noexcept;

	/**  Add easy handle to this multi handle */
	void add_handle(std::shared_ptr<Easy>) noexcept;

private:
	friend Context;
	void check_info() noexcept;
	static void start_timeout(CURLM*, long, Multi*) noexcept;
	static int handle_socket(CURL*, curl_socket_t, int, Multi*, Context*) noexcept;

	using Deleter = CURLMcode(*)(CURLM *);
	std::unique_ptr<CURLM, Deleter>
		_handle;
	std::shared_ptr<uvw::TimerHandle>
		_timer;
	std::shared_ptr<Global>
		_global;
};
}

#endif // UVW_CURL_MULTI_HPP
