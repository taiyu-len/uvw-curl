#ifndef UVW_CURL_EASY_HPP
#define UVW_CURL_EASY_HPP
#include "create_lock.hpp"
#include <curl/curl.h>
#include <memory>
#include <uvw/emitter.hpp>

namespace uvw_curl
{
struct Multi;
/**
 * Manages the CURL easy handle for use with uvw
 */
struct Easy
	: public CreateLock<Easy>
	, public uvw::Emitter<Easy>
	, public std::enable_shared_from_this<Easy>
{
	struct ErrorEvent
	{
		CURLcode code;
		const char* what() const noexcept { return curl_easy_strerror(code); }
	};
	struct EndEvent {};
	struct DataEvent { char const* data; size_t length; };
	struct XferInfoEvent { curl_off_t dltotal, dlnow, ultotal, ulnow; };

	Easy(Key) noexcept;
	~Easy() noexcept = default;

	// Curl Easy options
	void url(const char*) noexcept;
	const char*url() noexcept;

private:
	friend Multi;

	static size_t write(char*, size_t, size_t, Easy*) noexcept;
	static int xferinfo(
		Easy*,
		curl_off_t, curl_off_t, curl_off_t, curl_off_t) noexcept;
	void finish() noexcept;

	using Deleter = void(*)(CURL*);
	std::unique_ptr<CURL, Deleter>
		_handle;
	/**
	 * Self referential pointer set when owned by Multi handle and released
	 * before EndEvents are handled
	 */
	std::shared_ptr<Easy>
		_self;
	/**
	 * Pointer to owning multi handle
	 */
	std::shared_ptr<Multi>
		_multi;
};
}


#endif // UVW_CURL_EASY_HPP
