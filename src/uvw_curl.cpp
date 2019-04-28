#include "uvw_curl.hpp"
#include "log.hpp"

#include <iostream>
#include <uvw/poll.hpp>
#include <algorithm>

namespace uvw_curl
{

///////////////////////////////////////////////////////////////////////////////
// CurlGlobal

std::shared_ptr<CurlGlobal>
CurlGlobal::create(long flags)
{
	if (curl_global_init(flags)) {
		return nullptr;
	} else {
		return std::make_shared<CurlGlobal>();
	}
}

CurlGlobal::~CurlGlobal()
{
	curl_global_cleanup();
}

///////////////////////////////////////////////////////////////////////////////
// CurlMulti

static void start_timeout(CURLM*, long, void*) noexcept;
static int  handle_socket(CURL*, curl_socket_t, int, void*, void*) noexcept;

/// Initialize Curl Multi handle and timer
std::shared_ptr<CurlMulti>
CurlMulti::create(
	std::shared_ptr<uvw::Loop> loop,
	std::shared_ptr<CurlGlobal> global)
{
	auto multi = curl_multi_init();
	if (multi == nullptr) {
		return nullptr;
	}

	auto ptr = std::make_shared<CurlMulti>(multi);

	// set callback Functions
	ptr->setopt(CURLMOPT_SOCKETFUNCTION, handle_socket);
	ptr->setopt(CURLMOPT_TIMERFUNCTION, start_timeout);

	// set user data to contain pointer to multi handle
	ptr->setopt(CURLMOPT_SOCKETDATA, ptr.get());
	ptr->setopt(CURLMOPT_TIMERDATA, ptr.get());

	// enable multiplexing
	ptr->setopt(CURLMOPT_PIPELINING, CURLPIPE_HTTP1 | CURLPIPE_MULTIPLEX);

	ptr->timer = loop->resource<uvw::TimerHandle>();
	ptr->timer->on<uvw::TimerEvent>(
	[self = ptr.get()] (uvw::TimerEvent, uvw::TimerHandle const&) {
		int running_handles;
		curl_multi_socket_action(
			self->handle.get(), CURL_SOCKET_TIMEOUT, 0,
			&running_handles);
		self->check_info();
	});

	ptr->gstate = std::move(global);
	return ptr;
}

CurlMulti::CurlMulti(CURLM* multi) noexcept
: uvw::Emitter<CurlMulti>{}
, std::enable_shared_from_this<CurlMulti>{}
, handle( multi, &curl_multi_cleanup )
{}

CurlMulti::~CurlMulti() noexcept
{
	if (handle) {
		auto ptr = handle.release();
		auto err = curl_multi_cleanup(ptr);
		if (err) {
			publish(CurlMultiErrorEvent{err});
		}
	}
}

void CurlMulti::add_handle(std::shared_ptr<CurlEasy> easy)
{
	easy->multi = shared_from_this();
	easy->self  = easy->shared_from_this();
	easy->setopt(CURLOPT_PRIVATE, easy.get());

	auto err = curl_multi_add_handle(handle.get(), easy->handle.get());
	if (err) {
		publish(CurlMultiErrorEvent{err});
	}
}

void CurlMulti::check_info()
{
	CURLMsg *message;
	CurlEasy *easy;
	int pending;
	while ((message = curl_multi_info_read(handle.get(), &pending))) {
		switch (message->msg) {
		case CURLMSG_DONE:
			auto* easy_handle = message->easy_handle;
			curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &easy);
			curl_multi_remove_handle(handle.get(), easy_handle);
			easy->finish();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// CurlEasy
size_t CurlEasy::write_cb(
	char *ptr, size_t size, size_t nmemb, void* easyp)
{
	auto easy = static_cast<CurlEasy*>(easyp);
	easy->publish(CurlEasy::DataEvent{ptr, nmemb * size});
	return nmemb * size;
}

int CurlEasy::xfer_info_cb(
	void* easyp,
	curl_off_t dltotal, curl_off_t dlnow,
	curl_off_t ultotal, curl_off_t ulnow)
{
	auto easy = static_cast<CurlEasy*>(easyp);
	easy->publish(CurlEasy::XferInfoEvent{dltotal, dlnow, ultotal, ulnow});
	return 0;
}

CurlEasy::CurlEasy() noexcept
: uvw::Emitter<CurlEasy>{}
, std::enable_shared_from_this<CurlEasy>{}
, handle( curl_easy_init(), &curl_easy_cleanup )
{
	setopt(CURLOPT_WRITEFUNCTION, &write_cb);
	setopt(CURLOPT_WRITEDATA, this);
	setopt(CURLOPT_XFERINFOFUNCTION, &xfer_info_cb);
	setopt(CURLOPT_XFERINFODATA, this);
	setopt(CURLOPT_NOPROGRESS, 0L);
};

void CurlEasy::finish() noexcept
{
	auto ptr = shared_from_this();
	self.reset();
	publish(EndEvent{});
}

std::shared_ptr<CurlEasy>
CurlEasy::create()
{
	return std::make_shared<CurlEasy>();
}

///////////////////////////////////////////////////////////////////////////////
// CurlContext

/// Context for socket connection
struct CurlContext
{
	CurlContext(CurlMulti& multi, curl_socket_t s)
	: multi(multi)
	, s(s)
	, poll(multi.timer->loop().resource<uvw::PollHandle>(s))
	{
		// add this to socket state
		curl_multi_assign(multi.handle.get(), s, static_cast<void*>(this));
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
			curl_multi_socket_action(
				this->multi.handle.get(), this->s, flags,
				&running_handles);
			this->multi.check_info();
		});
	}
	~CurlContext() noexcept
	{
		curl_multi_assign(multi.handle.get(), s, nullptr);
	}
	CurlMulti&
		multi;
	curl_socket_t
		s;
	std::shared_ptr<uvw::PollHandle>
		poll;
};

/**
 * @param multi Pointer to Multi Handle
 * @param timeout_ms time in ms to call on_timeout callback
 * @param userp Pointer to CurlMulti
 */
void start_timeout(CURLM* handle, long timeout_ms, void* userp) noexcept
{
	auto& multi = *static_cast<CurlMulti*>(userp);
	if (timeout_ms == -1) {
		multi.timer->close();
		return;
	}
	if (timeout_ms == 0) {
		timeout_ms = 1;
	}
	auto timeout = uvw::TimerHandle::Time(timeout_ms);
	auto repeat  = uvw::TimerHandle::Time(0);
	multi.timer->start(timeout, repeat);
}

/**
 * @param easy Easy Handle
 * @param s    socket for connection
 * @param action what action we are taking on the socket
 * @param userp  CurlMulti*
 * @param socketp CurlContext* or nullptr
 */
int handle_socket(
	CURL* easy, curl_socket_t s, int action,
	void* userp, void* socketp) noexcept
{
	auto* multi   = static_cast<CurlMulti*>(userp);
	auto* context = static_cast<CurlContext*>(socketp);
	auto events = int();
	switch (action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
		// create context to start polling on
		if (! context) {
			context = new CurlContext(*multi, s);
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
}
