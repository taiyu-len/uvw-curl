#include <uvw/loop.hpp>
#include <uvw/timer.hpp>
#include <curl/curl.h>
#include <stdexcept>
#include <memory>
struct CurlGlobal; ///< Manages curl global state, used to create multi handles
struct CurlMulti;  ///< Manages curl multi handle and uvw timer
struct CurlEasy;   ///< Wrapper around a curl easy request

struct CurlGlobal
{
	CurlGlobal(long flags = CURL_GLOBAL_DEFAULT);
	~CurlGlobal();
};

struct CurlMulti
{
	CurlMulti(uvw::Loop& loop, CurlGlobal const&);
	~CurlMulti() noexcept;

	CurlMulti(CurlMulti const&) = delete;
	CurlMulti(CurlMulti &&) = delete;
	void operator=(CurlMulti const&) = delete;
	void operator=(CurlMulti &&) = delete;

	void add_handle(CurlEasy);
	void check_info();

	CURLM*
		handle;
	uvw::Loop&
		loop;
	std::shared_ptr<uvw::TimerHandle>
		timer;
};

struct CurlEasy
{
	CurlEasy() noexcept;
	CurlEasy(CurlEasy const& x) = delete;
	CurlEasy(CurlEasy&& x) noexcept;
	CurlEasy &operator=(CurlEasy const& x) = delete;
	CurlEasy &operator=(CurlEasy&& x) noexcept;

	~CurlEasy() noexcept;

	template<typename T>
	CURLcode setopt(CURLoption opt, T param) const noexcept
	{
		return curl_easy_setopt(handle, opt, param);
	}

	CURL* release() noexcept;
	void  reset() noexcept;

	CURL* handle;
};

