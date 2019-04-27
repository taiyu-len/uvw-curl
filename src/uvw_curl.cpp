#include "uvw_curl.hpp"
#include <iostream>
#include <uvw/poll.hpp>
#include <algorithm>
/// @param easy  handle for single request
/// @param s socket for request
/// @param action what action is being taken for request
/// @param userp Pointer to CurlMulti
/// @param socketp Pointer to CurlContext
static int uvwcurl_handle_socket(
	CURL* easy, curl_socket_t s, int action, void* userp, void* socketp);

///@param multi handle for all multi requests
///@param timeout_ms timeout in milliseconds
///@param userp contains pointer to CurlState
/// starts uv timer for multi handle
static void uvwcurl_start_timeout(CURLM* multi, long timeout_ms, void* userp);

CurlGlobal::CurlGlobal(long flags)
{
	printf("Init Curl Global State\n");
	if (curl_global_init(flags)) {
		throw std::runtime_error("Unable to initialize curl");
	}
}

CurlGlobal::~CurlGlobal()
{
	printf("Cleanup Curl Global State\n");
	curl_global_cleanup();
}

/// Initialize Curl Multi handle and timer
CurlMulti::CurlMulti(uvw::Loop& loop, CurlGlobal const&)
: loop(loop) {
	printf("Init Curl Multi Handle\n");
	if ((handle = curl_multi_init()) == nullptr) {
		throw std::runtime_error("curl multi init failed");
	}

	// setup socket and timer functions to use with uvw
	curl_multi_setopt(handle, CURLMOPT_SOCKETFUNCTION, uvwcurl_handle_socket);
	curl_multi_setopt(handle, CURLMOPT_TIMERFUNCTION, uvwcurl_start_timeout);

	// setup user data to contain pointer to this
	curl_multi_setopt(handle, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(handle, CURLMOPT_TIMERDATA,  this);

	// enable multiplexing
	curl_multi_setopt(handle, CURLMOPT_PIPELINING,
			  CURLPIPE_HTTP1 | CURLPIPE_MULTIPLEX);

	curl_multi_setopt(handle, CURLMOPT_MAXCONNECTS, 10L);
	timer = loop.resource<uvw::TimerHandle>();
	timer->on<uvw::TimerEvent>([this](uvw::TimerEvent, uvw::TimerHandle const& x) {
		int running_handles;
		curl_multi_socket_action(
			this->handle, CURL_SOCKET_TIMEOUT, 0,
			&running_handles);
		this->check_info();
	});
}

CurlMulti::~CurlMulti() noexcept
{
	printf("Cleanup Curl Multi Handle\n");
	curl_multi_cleanup(handle);
}

void CurlMulti::add_handle(CurlEasy ceasy)
{
	printf("Adding easy handle\n");
	auto* easy = ceasy.release();
	curl_multi_add_handle(handle, easy);
}

void CurlMulti::check_info()
{
	CURLMsg *message;
	int pending;
	while ((message = curl_multi_info_read(handle, &pending))) {
		switch (message->msg) {
		case CURLMSG_DONE:
			auto* easy_handle = message->easy_handle;
			curl_multi_remove_handle(handle, easy_handle);
			curl_easy_cleanup(easy_handle);
		}
	}
}

CurlEasy::CurlEasy() noexcept
: handle( curl_easy_init() )
{};

CurlEasy::CurlEasy(CurlEasy&& x) noexcept
{
	handle = x.handle;
	x.handle = nullptr;
}

CurlEasy& CurlEasy::operator=(CurlEasy&& x) noexcept
{
	if (this != &x) {
		if (handle) {
			curl_easy_cleanup(handle);
		}
		handle = x.handle;
		x.handle = nullptr;
	}
	return *this;
}

CurlEasy::~CurlEasy() noexcept
{
	if (handle) {
		curl_easy_cleanup(handle);
	}
}

CURL* CurlEasy::release() noexcept
{
	auto x = handle;
	handle = nullptr;
	return x;
}

void CurlEasy::reset() noexcept
{
	handle = curl_easy_init();
}

/// Context for easy connection
struct CurlContext
{
	CurlContext(CurlMulti& state, curl_socket_t s)
	: state(state)
	, s(s)
	, poll(state.loop.resource<uvw::PollHandle>(s))
	{
		printf("Create Context: socket(%d)\n", s);
		// add this to socket state
		curl_multi_assign(state.handle, s, static_cast<void*>(this));
		// delete self in poll close event
		poll->on<uvw::CloseEvent>(
		[this] (uvw::CloseEvent const&, uvw::PollHandle const&) {
			delete this;
		});
		// handle socket events
		poll->on<uvw::PollEvent>(
		[this] (uvw::PollEvent const& e, uvw::PollHandle const&) {
			int running_handles;
			int flags = 0;
			if (e.flags & uvw::PollHandle::Event::READABLE) {
				flags |= CURL_CSELECT_IN;
			}
			if (e.flags & uvw::PollHandle::Event::WRITABLE) {
				flags |= CURL_CSELECT_OUT;
			}
			printf("PollEvent %d (%d)\n", flags, this->s);
			curl_multi_socket_action(
				this->state.handle, this->s, flags,
				&running_handles);
			this->state.check_info();
		});
	}
	~CurlContext() noexcept
	{
		printf("Delete Context\n");
		// remove this context for socket s
		curl_multi_assign(state.handle, s, nullptr);
	}
	CurlMulti&
		state;
	curl_socket_t
		s;
	std::shared_ptr<uvw::PollHandle>
		poll;
};

int uvwcurl_handle_socket(
	CURL* easy, curl_socket_t s, int action, void* userp, void* socketp)
{
	auto* state   = static_cast<CurlMulti*>(userp);
	auto* context = static_cast<CurlContext*>(socketp);
	auto events = int();
	switch (action) {
	case CURL_POLL_IN:
		printf("POLL_IN (%d)\n", s); break;
	case CURL_POLL_OUT:
		printf("POLL_OUT (%d)\n", s); break;
	case CURL_POLL_INOUT:
		printf("POLL_INOUT (%d)\n", s); break;
	case CURL_POLL_REMOVE:
		printf("POLL_REMOVE (%d)\n", s); break;
	}

	switch (action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
		// create context to start polling on
		if (! context) {
			context = new CurlContext(*state, s);
		}
		if (action != CURL_POLL_IN) {
			events |= UV_WRITABLE;
		}
		if (action != CURL_POLL_OUT) {
			events |= UV_READABLE;
		}
		context->poll->start(events);
		break;
	case CURL_POLL_REMOVE:
		// stop polling and close it
		context->poll->stop();
		context->poll->close();
		break;
	}
	return 0;
}

void uvwcurl_start_timeout(CURLM* multi, long timeout_ms, void* userp)
{
	auto& state = *static_cast<CurlMulti*>(userp);
	if (timeout_ms == -1) {
		state.timer->close();
		return;
	}
	// set minimum timeout
	if (timeout_ms == 0) {
		timeout_ms = 1;
	}
	auto timeout = uvw::TimerHandle::Time(timeout_ms);
	auto repeat  = uvw::TimerHandle::Time(0);
	state.timer->start(timeout, repeat);
}
