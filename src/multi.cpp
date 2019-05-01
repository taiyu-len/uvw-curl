#include "context.hpp"
#include "easy.hpp"
#include "global.hpp"
#include "multi.hpp"

namespace uvw_curl
{
Multi::Multi(
	Key,
	std::shared_ptr<uvw::Loop> const& loop,
	std::shared_ptr<Global> global) noexcept
: uvw::Emitter<Multi>()
, std::enable_shared_from_this<Multi>()
, _handle(curl_multi_init(), &curl_multi_cleanup)
, _timer(loop->resource<uvw::TimerHandle>())
, _global(std::move(global))
{
	// set up callbacks
	curl_multi_setopt(_handle.get(), CURLMOPT_SOCKETFUNCTION, handle_socket);
	curl_multi_setopt(_handle.get(), CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(_handle.get(), CURLMOPT_TIMERFUNCTION, start_timeout);
	curl_multi_setopt(_handle.get(), CURLMOPT_TIMERDATA, this);

	// setup timer event handler
	_timer->on<uvw::TimerEvent>(
	[this] (auto const&, auto const&) {
		int running;
		auto err = curl_multi_socket_action(
			_handle.get(), CURL_SOCKET_TIMEOUT, 0, &running);
		if (err)
		{
			publish(ErrorEvent{err});
		}
		check_info();
	});
}

Multi::~Multi() noexcept
{
	auto ptr = _handle.release();
	if (ptr)
	{
		auto err = curl_multi_cleanup(ptr);
		if (err)
		{
			publish(ErrorEvent{err});
		}
	}
}

bool Multi::init() const noexcept
{
	return bool(_handle);
}

void Multi::add_handle(std::shared_ptr<Easy> easy) noexcept
{
	easy->_multi = shared_from_this();
	curl_easy_setopt(easy->_handle.get(), CURLOPT_PRIVATE, easy.get());
	auto err = curl_multi_add_handle(_handle.get(), easy->_handle.get());
	if (err != 0)
	{
		publish(ErrorEvent{err});
	}
	else
	{
		easy->_self = std::move(easy);
	}
}

/**
 * Checks messages returned from curl_multi_info_read and issues finish events
 * for completed easy handles
 */
void Multi::check_info() noexcept
{
	CURLMsg  *message;
	int pending;
	while ((message = curl_multi_info_read(_handle.get(), &pending)))
	{
		if (message->msg == CURLMSG_DONE)
		{
			auto* easy_handle = message->easy_handle;
			Easy* easy;
			curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &easy);
			auto err = curl_multi_remove_handle(_handle.get(), easy_handle);
			publish(ErrorEvent{err});
			easy->finish();
		}
	}
}

void Multi::start_timeout(CURLM*, long timeout, Multi* multi) noexcept
{
	if (timeout == -1)
	{
		multi->_timer->close();
		return;
	}
	if (timeout == 0)
	{
		timeout = 1;
	}
	multi->_timer->start(
		uvw::TimerHandle::Time(timeout),
		uvw::TimerHandle::Time(0));
}

int Multi::handle_socket(
	CURL*, curl_socket_t s, int action,
	Multi* multi, Context* context) noexcept
{
	int events = 0;
	switch (action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
		// create context to start polling on
		if (context == nullptr) {
			context = Context::create_raw(*multi, s);
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
	default: break;
	}
	return 0;
}
} // namespace uvw_curl
