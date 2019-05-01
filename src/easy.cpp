#include "easy.hpp"
#include "multi.hpp"

namespace uvw_curl
{
struct Easy::Callback {
	static size_t write(char*, size_t, size_t, Easy*) noexcept;
	static int xferinfo(Easy*, curl_off_t, curl_off_t, curl_off_t, curl_off_t) noexcept;
	static int seek(Easy*, curl_off_t ofset, int origin) noexcept;
	static size_t header(char*, size_t, size_t, Easy*) noexcept;
	static int debug(CURL*, curl_infotype, char*, size_t, Easy*) noexcept;
};

Easy::Easy(Key) noexcept
: uvw::Emitter<Easy>()
, std::enable_shared_from_this<Easy>()
, error_buffer({0})
, _handle(curl_easy_init(), &curl_easy_cleanup)
{
	curl_easy_setopt(_handle.get(), CURLOPT_WRITEFUNCTION, &Callback::write);
	curl_easy_setopt(_handle.get(), CURLOPT_WRITEDATA, this);

	curl_easy_setopt(_handle.get(), CURLOPT_XFERINFOFUNCTION, &Callback::xferinfo);
	curl_easy_setopt(_handle.get(), CURLOPT_XFERINFODATA, this);

	curl_easy_setopt(_handle.get(), CURLOPT_DEBUGFUNCTION, &Callback::debug);
	curl_easy_setopt(_handle.get(), CURLOPT_DEBUGDATA, this);

	curl_easy_setopt(_handle.get(), CURLOPT_ERRORBUFFER, error_buffer.data());
}

void Easy::seek_events(bool x) noexcept
{
	auto cb = x ? &Callback::seek : nullptr;
	auto dt = x ? this : nullptr;
	curl_easy_setopt(_handle.get(), CURLOPT_SEEKFUNCTION, cb);
	curl_easy_setopt(_handle.get(), CURLOPT_SEEKDATA,     dt);
}

void Easy::header_events(bool x) noexcept
{
	auto cb = x ? &Callback::header : nullptr;
	auto dt = x ? this : nullptr;
	curl_easy_setopt(_handle.get(), CURLOPT_HEADERFUNCTION, cb);
	curl_easy_setopt(_handle.get(), CURLOPT_HEADERDATA,     dt);
}

#define SET_OPT(name, code, type, arg_type) \
void Easy::name(arg_type x) noexcept { \
	auto err = curl_easy_setopt(_handle.get(), CURLOPT_##code, type(x)); \
	if (err != 0) { publish(ErrorEvent{err}); }\
}
static auto _get_cstring(zstring x) -> const char*
{
	return x;
}

static auto _long_not(bool x) -> long
{
	return long(!x);
}

SET_OPT(url           , URL            , _get_cstring     , zstring);
SET_OPT(progress      , NOPROGRESS     , _long_not        , bool);
SET_OPT(verbose       , VERBOSE        , long             , bool);
SET_OPT(header        , HEADER         , long             , bool);
SET_OPT(signal        , NOSIGNAL       , _long_not        , bool);
SET_OPT(wildcard_match, WILDCARDMATCH  , long             , bool);
SET_OPT(path_as_is    , PATH_AS_IS     , long             , bool);
SET_OPT(netrc         , NETRC          , static_cast<long>, netrc_level);
SET_OPT(netrc_file    , NETRC_FILE     , _get_cstring     , zstring);
SET_OPT(userpwd       , USERPWD        , _get_cstring     , zstring);
SET_OPT(proxyuserpwd  , PROXYUSERPWD   , _get_cstring     , zstring);

SET_OPT(username      , USERNAME       , _get_cstring     , zstring);
SET_OPT(password      , PASSWORD       , _get_cstring     , zstring);
SET_OPT(login_options , LOGIN_OPTIONS  , _get_cstring     , zstring);
SET_OPT(proxyusername , PROXYUSERNAME  , _get_cstring     , zstring);
SET_OPT(proxypassword , PROXYPASSWORD  , _get_cstring     , zstring);
SET_OPT(httpauth      , HTTPAUTH       , static_cast<long>, http_auth);
#undef SET_OPT
#define GET_INFO(name, code, type, ret_type) \
auto Easy::name() noexcept -> ret_type { \
	type x; \
	auto err = curl_easy_getinfo(_handle.get(), CURLINFO_##code, &x); \
	if (err != 0) { publish(ErrorEvent{err}); } \
	return ret_type(x); \
}
GET_INFO(effective_url    , EFFECTIVE_URL    , char*     , std::string);
GET_INFO(response_code    , RESPONSE_CODE    , long      , long);
GET_INFO(http_connectcode , HTTP_CONNECTCODE , long      , long);
GET_INFO(http_version     , HTTP_VERSION     , long      , long);
GET_INFO(filetime         , FILETIME         , long      , long);
GET_INFO(filetime_t       , FILETIME_T       , curl_off_t, curl_off_t);
GET_INFO(total_time       , TOTAL_TIME       , double    , double);
GET_INFO(total_time_t     , TOTAL_TIME_T     , curl_off_t, curl_off_t);
GET_INFO(namelookup_time  , NAMELOOKUP_TIME  , double    , double);
GET_INFO(namelookup_time_t, NAMELOOKUP_TIME_T, curl_off_t, curl_off_t);
GET_INFO(connect_time     , CONNECT_TIME     , double    , double);

GET_INFO(connect_time_t      , CONNECT_TIME_T      , curl_off_t, curl_off_t);
GET_INFO(appconnect_time     , APPCONNECT_TIME     , double    , double);
GET_INFO(appconnect_time_t   , APPCONNECT_TIME_T   , curl_off_t, curl_off_t);
GET_INFO(pretransfer_time    , PRETRANSFER_TIME    , double    , double);
GET_INFO(pretransfer_time_t  , PRETRANSFER_TIME_T  , curl_off_t, curl_off_t);
GET_INFO(starttransfer_time  , STARTTRANSFER_TIME  , double    , double);
GET_INFO(starttransfer_time_t, STARTTRANSFER_TIME_T, curl_off_t, curl_off_t);
GET_INFO(redirect_time       , REDIRECT_TIME       , double    , double);
GET_INFO(redirect_time_t     , REDIRECT_TIME_T     , curl_off_t, curl_off_t);
GET_INFO(redirect_count      , REDIRECT_COUNT      , long      , long);
GET_INFO(redirect_url        , REDIRECT_URL        , char*     , std::string);

GET_INFO(size_upload            , SIZE_UPLOAD_T            , curl_off_t , curl_off_t);
GET_INFO(size_download          , SIZE_DOWNLOAD_T          , curl_off_t , curl_off_t);
GET_INFO(speed_download         , SPEED_DOWNLOAD_T         , curl_off_t , curl_off_t);
GET_INFO(speed_upload           , SPEED_UPLOAD_T           , curl_off_t , curl_off_t);
GET_INFO(header_size            , HEADER_SIZE              , long       , long);
GET_INFO(request_size           , REQUEST_SIZE             , long       , long);
GET_INFO(ssl_verifyresult       , SSL_VERIFYRESULT         , long       , long);
GET_INFO(proxy_ssl_verifyresult , PROXY_SSL_VERIFYRESULT   , long       , long);
GET_INFO(ssl_engines            , SSL_ENGINES              , curl_slist*, List);
GET_INFO(content_length_download, CONTENT_LENGTH_DOWNLOAD_T, curl_off_t , curl_off_t);
GET_INFO(content_length_upload  , CONTENT_LENGTH_UPLOAD_T  , curl_off_t , curl_off_t);
#undef GET_INFO

/**
 * emits EndEvent, and releases ownership of self.
 */
void Easy::finish() noexcept
{
	auto ptr = shared_from_this();
	_self.reset();
	publish(EndEvent{});
}

size_t Easy::Callback::write(char* data, size_t size, size_t nmemb, Easy *easy) noexcept
{
	easy->publish(Easy::DataEvent{data, nmemb * size});
	return nmemb * size;
}

int Easy::Callback::xferinfo(
	Easy* easy,
	curl_off_t dltotal, curl_off_t dlnow,
	curl_off_t ultotal, curl_off_t ulnow) noexcept
{
	easy->publish(Easy::XferEvent{dltotal, dlnow, ultotal, ulnow});
	return 0;
}

int Easy::Callback::seek(Easy* easy, curl_off_t offset, int origin) noexcept
{
	easy->publish(Easy::SeekEvent{offset, origin});
	return 0;
}

size_t Easy::Callback::header(char* data, size_t nmemb, size_t size, Easy* easy) noexcept
{
	easy->publish(Easy::HeaderEvent{data, nmemb * size});
	return nmemb * size;
}

int Easy::Callback::debug(
	CURL*, curl_infotype type, char* data, size_t size,
	Easy *easy) noexcept
{
	easy->publish(Easy::DebugEvent{data, size, type});
	return 0;
}
} // namespace uvw_curl
