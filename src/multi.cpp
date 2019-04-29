#include "easy.hpp"
#include "global.hpp"
#include "multi.hpp"
#include "context.hpp"
#include "log.hpp"

namespace uvw_curl
{
auto Multi::create(
	std::shared_ptr<uvw::Loop> loop,
	std::shared_ptr<Global> global)
	-> std::shared_ptr<Multi>
{
	TRACE() << "Creating multi";
	auto multi = curl_multi_init();
	if (multi == nullptr)
	{
		return nullptr;
	}
	auto ptr = std::make_shared<Multi>(Key{});
	ptr->_handle.reset(multi);

	// set up callbacks
	curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, handle_socket);
	curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, ptr.get());
	curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, start_timeout);
	curl_multi_setopt(multi, CURLMOPT_TIMERDATA, ptr.get());

	// setup timer event handler
	ptr->_timer = loop->resource<uvw::TimerHandle>();
	ptr->_timer->on<uvw::TimerEvent>(
	[self = ptr.get()] (auto const&, auto const&) {
		int running;
		curl_multi_socket_action(
			self->_handle.get(), CURL_SOCKET_TIMEOUT, 0, &running);
		self->check_info();
	});

	ptr->_global = std::move(global);
	return ptr;
}

Multi::Multi(Key x) noexcept
: CreateLock<Multi>(x)
, uvw::Emitter<Multi>()
, std::enable_shared_from_this<Multi>()
, _handle(nullptr, &curl_multi_cleanup)
{}

Multi::~Multi() noexcept
{
	TRACE() << "Destroying multi";
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

void Multi::add_handle(std::shared_ptr<Easy> easy) noexcept
{
	TRACE() << "Add Handle";
	easy->_multi = shared_from_this();
	easy->_self  = easy->shared_from_this();
	curl_easy_setopt(easy->_handle.get(), CURLOPT_PRIVATE, easy.get());
	auto err = curl_multi_add_handle(_handle.get(), easy->_handle.get());
	if (err)
	{
		publish(ErrorEvent{err});
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
		switch (message->msg)
		{
		case CURLMSG_DONE:
			auto* easy_handle = message->easy_handle;
			Easy* easy;
			curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &easy);
			curl_multi_remove_handle(_handle.get(), easy_handle);
			easy->finish();
			break;
		}
	}
}

void Multi::start_timeout(CURLM* handle, long timeout, Multi* multi) noexcept
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
	CURL* easy, curl_socket_t s, int action,
	Multi* multi, Context* context) noexcept
{
	TRACE() << "Handle socket " << s;
	int events = 0;
	switch (action) {
	case CURL_POLL_IN:
	case CURL_POLL_OUT:
	case CURL_POLL_INOUT:
		// create context to start polling on
		if (context == nullptr) {
			context = new Context(*multi, s);
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
