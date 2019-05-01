#ifndef UVW_CURL_EASY_HPP
#define UVW_CURL_EASY_HPP
#include "create_lock.hpp"
#include "list.hpp"
#include "zstring.hpp"
#include <array>
#include <curl/curl.h>
#include <memory>
#include <string>
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
	/* Events */
	struct ErrorEvent {
		CURLcode code;
		const char* what() const noexcept { return curl_easy_strerror(code); }
	};
	struct EndEvent {};
	struct DataEvent { char const* data; size_t length; };
	struct XferEvent { curl_off_t dltotal, dlnow, ultotal, ulnow; };
	struct SeekEvent { curl_off_t offset; int origin; };
	struct HeaderEvent { char const* data; size_t length; };
	struct DebugEvent { char const* data; size_t length; curl_infotype type; };
	struct Callback;

	/* Enums for options/info */
	enum class netrc_level : long {
		optional = CURL_NETRC_OPTIONAL,
		ignored  = CURL_NETRC_IGNORED,
		required = CURL_NETRC_REQUIRED
	};
	enum http_auth : unsigned long {
		basic     = CURLAUTH_BASIC,
		digest    = CURLAUTH_DIGEST,
		digest_ie = CURLAUTH_DIGEST_IE,
		bearer    = CURLAUTH_BEARER,
		negotiate = CURLAUTH_NEGOTIATE,
		ntlm      = CURLAUTH_NTLM,
		ntlm_wb   = CURLAUTH_NTLM_WB,
		any       = CURLAUTH_ANY,
		anysafe   = CURLAUTH_ANYSAFE,
		only      = CURLAUTH_ONLY,
	};

	Easy(Key) noexcept;
	~Easy() noexcept = default;

	// Enable/Disable certain events
	void progress(bool) noexcept; // enables xfer events
	void verbose(bool)  noexcept; // enables debug events
	void seek_events(bool) noexcept; // enable seek events
	void header_events(bool) noexcept; // enable header events

	// Curl Easy options
	void url(zstring) noexcept;
	void header(bool) noexcept;
	void signal(bool) noexcept;
	void wildcard_match(bool) noexcept; // TODO test how it works with multi
	void path_as_is(bool) noexcept;

	void netrc(netrc_level) noexcept;
	void netrc_file(zstring) noexcept; // default uses netrc in ~
	void userpwd(zstring) noexcept; // "username:password"
	void proxyuserpwd(zstring) noexcept;
	void username(zstring) noexcept;
	void password(zstring) noexcept;
	void login_options(zstring) noexcept;
	void proxyusername(zstring) noexcept;
	void proxypassword(zstring) noexcept;
	void httpauth(http_auth) noexcept;

	// See curl_Easy_getinfo
	auto effective_url()                   noexcept -> std::string;
	auto response_code()                   noexcept -> long;
	auto http_connectcode()                noexcept -> long;
	auto http_version()                    noexcept -> long;
	auto filetime()                        noexcept -> long;
	auto filetime_t()                      noexcept -> curl_off_t;
	auto total_time()                      noexcept -> double;
	auto total_time_t()                    noexcept -> curl_off_t;
	auto namelookup_time()                 noexcept -> double;
	auto namelookup_time_t()               noexcept -> curl_off_t;
	auto connect_time()                    noexcept -> double;
	auto connect_time_t()                  noexcept -> curl_off_t;
	auto appconnect_time()                 noexcept -> double;
	auto appconnect_time_t()               noexcept -> curl_off_t;
	auto pretransfer_time()                noexcept -> double;
	auto pretransfer_time_t()              noexcept -> curl_off_t;
	auto starttransfer_time()              noexcept -> double;
	auto starttransfer_time_t()            noexcept -> curl_off_t;
	auto redirect_time()                   noexcept -> double;
	auto redirect_time_t()                 noexcept -> curl_off_t;
	auto redirect_count()                  noexcept -> long;
	auto redirect_url()                    noexcept -> std::string;
	auto size_upload()                     noexcept -> curl_off_t;
	auto size_download()                   noexcept -> curl_off_t;
	auto speed_download()                  noexcept -> curl_off_t;
	auto speed_upload()                    noexcept -> curl_off_t;
	auto header_size()                     noexcept -> long;
	auto request_size()                    noexcept -> long;
	auto ssl_verifyresult()                noexcept -> curl_off_t;
	auto proxy_ssl_verifyresult()          noexcept -> curl_off_t;
	auto ssl_engines()                     noexcept -> List;
	auto content_length_download()         noexcept -> curl_off_t;
	auto content_length_upload()           noexcept -> curl_off_t;
	// TODO finish these

	std::array<char, CURL_ERROR_SIZE>
		error_buffer;

	/**
	 * Pointer to owning multi handle
	 */
	std::shared_ptr<Multi>
		multi;
private:
	friend Multi;

	// Callbacks
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
};
}


#endif // UVW_CURL_EASY_HPP
