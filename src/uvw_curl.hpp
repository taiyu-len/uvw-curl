#include <uvw/loop.hpp>
#include <uvw/timer.hpp>
#include <curl/curl.h>
#include <stdexcept>
#include <memory>
// uvw design.
// The template parameters are
// - T: The Complete Handle Type.
// - U: The Type being wrapped
//
// UnderlyingType<T, U> contains
// - shared_ptr to uvw::Loop
// - U
//
// Emitter<T> contains
// - vector of handlers for each event.
// nothing special here.
//
// Resource<T, U>: UnderlyingType<T, U>, Emitter<T>, shared_from_this<T>
// contains
// - userData not used by uvw
// - sPtr shared pointer to self, and weak pointer to self
// Does
// - U->data = &T : underlying type requires data pointer to self
//
// Handle<T, U>: BaseHandle, Resource<T, U>
// very specific to uv_lib types, possibly modify for Curl Usage


namespace uvw_curl {
struct CurlGlobal; ///< Resource wrapping curl global state
struct CurlMulti;  ///< Resource wrapping multi handle and uv timer
struct CurlEasy;   ///< Resource representing a easy request

/// Curl Error Events
struct CurlMultiErrorEvent {
	CURLMcode error;
};
struct CurlErrorEvent {
	CURLcode error;
};

struct CurlGlobal : public std::enable_shared_from_this<CurlGlobal>
{
	static std::shared_ptr<CurlGlobal>
		create(long flags = CURL_GLOBAL_DEFAULT);
	~CurlGlobal();
	CurlGlobal() {};
};

// Emitted events:
// - CurlMultiErrorEvent
struct CurlMulti
	: public uvw::Emitter<CurlMulti>
	, public std::enable_shared_from_this<CurlMulti>
{

	static std::shared_ptr<CurlMulti>
	create(std::shared_ptr<uvw::Loop>, std::shared_ptr<CurlGlobal>);

	virtual ~CurlMulti() noexcept;
	CurlMulti(CURLM* m) noexcept;

	CurlMulti(CurlMulti &&) = default;
	CurlMulti& operator=(CurlMulti &&) = default;

	/// Add Curl Easy request to be performed by this multi handle
	void add_handle(std::shared_ptr<CurlEasy>);

	/** Set Option for multi handles.
	 * @param option Should not be any of
	 * - CURLMOPT_SOCKETFUNCTION
	 * - CURLMOPT_TIMERFUNCTION
	 * - CURLMOPT_SOCKETDATA
	 * - CURLMOPT_TIMERDATA.
	 * @param param value for option
	 */
	template<typename T>
	CURLMcode setopt(CURLMoption option, T param) const noexcept
	{
		return curl_multi_setopt(handle.get(), option, param);
	}

	void check_info();

	using Deleter = CURLMcode(*)(CURLM *);
	std::unique_ptr<CURLM, Deleter>
		handle;
	std::shared_ptr<uvw::TimerHandle>
		timer;
	std::shared_ptr<CurlGlobal>
		gstate;
};

/**
 * Easy Request Handle.
 *
 *
 * Emitted Events
 * - DataEvent:
 *   Recieve some sequence of bytes
 *
 * - EndEvent:
 *   Emitted when download/upload has completed.
 *   will reset sptr to self.
 *   can setopt/resubmit easy handle to multi handle here if want to reuse
 *
 * - XferInfoEvent
 *   dl/ul total and amounts
 *
 * - TODO add more events
 */
struct CurlEasy
	: public uvw::Emitter<CurlEasy>
	, public std::enable_shared_from_this<CurlEasy>
{
	struct EndEvent {};
	struct DataEvent { char const* data; size_t length; };
	struct XferInfoEvent { curl_off_t dltotal, dlnow, ultotal, ulnow; };

	static std::shared_ptr<CurlEasy>
		create();

	CurlEasy() noexcept;
	~CurlEasy() noexcept = default;

	/// Set options for curl easy request
	template<typename T>
	CURLcode setopt(CURLoption opt, T param) const noexcept {
		return curl_easy_setopt(handle.get(), opt, param);
	}
	template<typename T>
	T getinfo(CURLINFO info) const noexcept {
		T x;
		curl_easy_getinfo(handle.get(), info, &x);
		return x;
	}

	/// handle EndEvent, called by CurlMulti handle
	void finish() noexcept;

	static size_t write_cb(char*, size_t, size_t, void*);
	static int xfer_info_cb(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t);

	using Deleter = void(*)(CURL*);
	std::unique_ptr<CURL, Deleter>
		handle;
	/// Self referential shared pointer, set when passed to CurlMulti
	/// reset after EndEvent is handled
	std::shared_ptr<CurlEasy>
		self;
	std::shared_ptr<CurlMulti>
		multi;
};

}

